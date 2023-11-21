//
// SPDX-FileCopyrightText: Copyright 2023 Arm Limited and/or its affiliates <open-source-office@arm.com>
// SPDX-License-Identifier: MIT
//

#include <string.h>
#include <stdint.h>
#include <stdio.h>

#include "cmsis_gcc.h"

#include "hardware/clocks.h"
#include "hardware/dma.h"
#include "hardware/pwm.h"

#include "pico/hm01b0.h"

#define HM01B0_I2C_ADDRESS 0x24

static struct hm01b0_inst_t {
    struct hm01b0_config config;
    uint pio_program_offset;
    pio_sm_config pio_sm_config;
    uint num_pclk_per_px;
} hm01b0_inst;

static int hm01b0_reset();

static uint8_t hm01b0_read_reg8(uint16_t address);
static uint16_t hm01b0_read_reg16(uint16_t address);
static void hm01b0_write_reg8(uint16_t address, uint8_t value);
static void hm01b0_write_reg16(uint16_t address, uint16_t value);

int hm01b0_init(const struct hm01b0_config* config)
{
    memcpy(&hm01b0_inst.config, config, sizeof(hm01b0_inst.config));

    uint8_t readout_x_val;;          // 0x0383
    uint8_t readout_y_val;           // 0x0387
    uint8_t binning_mode_val;        // 0x0390
    uint8_t qvga_win_en_val;         // 0x3010
    uint16_t frame_length_lines_val; // 0x0340
    uint16_t line_length_pclk_val;   // 0x0342
    uint8_t bit_control_val;         // 0x3059

    uint8_t num_border_px;

    if (config->width == 320 && config->height == 320) {
        readout_x_val          = 0x01;
        readout_y_val          = 0x01;
        binning_mode_val       = 0x00;
        qvga_win_en_val        = 0x00;
        frame_length_lines_val = 0x0158;
        line_length_pclk_val   = 0x0178;

        num_border_px = 2;
    } else if (config->width == 320 && config->height == 240) {
        readout_x_val          = 0x01;
        readout_y_val          = 0x01;
        binning_mode_val       = 0x00;
        qvga_win_en_val        = 0x01;
        frame_length_lines_val = 0x0104;
        line_length_pclk_val   = 0x0178;

        num_border_px = 2;
    } else if (config->width == 160 && config->height == 120) {
        readout_x_val          = 0x03;
        readout_y_val          = 0x03;
        binning_mode_val       = 0x03;
        qvga_win_en_val        = 0x01;
        frame_length_lines_val = 0x0080;
        line_length_pclk_val   = 0x00D7;

        num_border_px = 2;
    } else {
        printf("Invalid resolution!\n");
        return -1;
    }

    if (config->data_bits == 8) {
        bit_control_val = 0x02;
        hm01b0_inst.num_pclk_per_px = 1;
    } else if (config->data_bits == 4) {
        bit_control_val = 0x42;
        hm01b0_inst.num_pclk_per_px = 2;
    } else if (config->data_bits == 1) {
        bit_control_val = 0x22;
        hm01b0_inst.num_pclk_per_px = 8;
    } else {
        printf("Invalid data bits!\n");
        return -1;
    }

    if (config->reset_pin > -1) {
        gpio_init(config->reset_pin);
        gpio_set_dir(config->reset_pin, GPIO_OUT);
        gpio_put(config->reset_pin, 0);
        sleep_ms(100);
        gpio_put(config->reset_pin, 1);
    }

    if (config->mclk_pin > -1) {
        gpio_set_function(config->mclk_pin, GPIO_FUNC_PWM);
        uint mclk_slice_num = pwm_gpio_to_slice_num(config->mclk_pin);
        uint mclk_channel = pwm_gpio_to_channel(config->mclk_pin);

        // PWM @ ~25 MHz, 50% duty cycle
        pwm_set_clkdiv(mclk_slice_num, 1.25);
        pwm_set_wrap(mclk_slice_num, 3);
        pwm_set_chan_level(mclk_slice_num, mclk_channel, 2);
        pwm_set_enabled(mclk_slice_num, true);
    }

    gpio_set_function(config->sda_pin, GPIO_FUNC_I2C);
    gpio_set_function(config->scl_pin, GPIO_FUNC_I2C);
    gpio_pull_up(config->sda_pin);
    gpio_pull_up(config->scl_pin);

    i2c_init(config->i2c, 100 * 1000);

    uint16_t model_id = hm01b0_read_reg16(0x0000);
    if (model_id != 0x01b0) {
        printf("Invalid model id!\n");
        return -1;
    }

    if (hm01b0_reset() != 0) {
        printf("Reset failed!\n");
        return -1;
    }

    hm01b0_write_reg8(0x3059, bit_control_val);

    hm01b0_write_reg8(0x0383, readout_x_val);
    hm01b0_write_reg8(0x0387, readout_y_val);
    hm01b0_write_reg8(0x0390, binning_mode_val);
    hm01b0_write_reg8(0x3010, qvga_win_en_val);
    hm01b0_write_reg16(0x0340, frame_length_lines_val);
    hm01b0_write_reg16(0x0342, line_length_pclk_val);
    

    hm01b0_write_reg8(0x3060, 0x08 | 0); // OSC_CLK_DIV

    hm01b0_write_reg16(0x0202, line_length_pclk_val / 2); // INTEGRATION_H

    hm01b0_write_reg8(0x0104, 0x01); // GRP_PARAM_HOLD

    pio_program_t pio_program;
    uint16_t pio_program_instructions[] = {
        /* 0 */ pio_encode_pull(false, true),
        /* 1 */ pio_encode_wait_gpio(false, config->vsync_pin),
        /* 2 */ pio_encode_wait_gpio(true, config->vsync_pin),
        /* 3 */ pio_encode_set(pio_y, num_border_px - 1),
        /* 4 */ pio_encode_wait_gpio(true, config->hsync_pin), // border pixel y
        /* 5 */ pio_encode_wait_gpio(false, config->hsync_pin),
        /* 6 */ pio_encode_jmp_y_dec(4),
        /* .wrap_target */
        /* 7 */ pio_encode_mov(pio_x, pio_osr),
        /* 8 */ pio_encode_wait_gpio(true, config->hsync_pin),
        /* 9 */ pio_encode_set(pio_y, num_border_px * hm01b0_inst.num_pclk_per_px - 1),
        /* 10 */ pio_encode_wait_gpio(true, config->pclk_pin), // border pixel x
        /* 11 */ pio_encode_wait_gpio(false, config->pclk_pin),
        /* 12 */ pio_encode_jmp_y_dec(10),
        /* 13 */ pio_encode_wait_gpio(true, config->pclk_pin),
        /* 14 */ pio_encode_in(pio_pins, config->data_bits),
        /* 15 */ pio_encode_wait_gpio(false, config->pclk_pin),
        /* 16 */ pio_encode_jmp_x_dec(13),
        /* 17 */ pio_encode_wait_gpio(false, config->hsync_pin),
        /* .wrap */
    };

    pio_program.instructions = pio_program_instructions;
    pio_program.length = sizeof(pio_program_instructions) / sizeof(pio_program_instructions[0]);
    pio_program.origin = -1;

    hm01b0_inst.pio_program_offset = pio_add_program(config->pio, &pio_program);
    hm01b0_inst.pio_sm_config = pio_get_default_sm_config();

    sm_config_set_in_pins(&hm01b0_inst.pio_sm_config, config->data_pin_base);
    sm_config_set_in_shift(&hm01b0_inst.pio_sm_config, true, true, 8);
    sm_config_set_wrap(
        &hm01b0_inst.pio_sm_config,
        hm01b0_inst.pio_program_offset + 7,
        hm01b0_inst.pio_program_offset + pio_program.length - 1
    );

    pio_gpio_init(config->pio, config->vsync_pin);
    pio_gpio_init(config->pio, config->hsync_pin);
    pio_gpio_init(config->pio, config->pclk_pin);

    return 0;
}

void hm01b0_deinit()
{
    struct hm01b0_config* config = &hm01b0_inst.config;

    i2c_deinit(config->i2c);

    gpio_set_function(config->sda_pin, GPIO_FUNC_NULL);
    gpio_set_function(config->scl_pin, GPIO_FUNC_NULL);

    if (config->mclk_pin > -1) {
        gpio_set_function(config->mclk_pin, GPIO_FUNC_NULL);
    }

    if (config->reset_pin > -1) {
        gpio_set_function(config->reset_pin, GPIO_FUNC_NULL);
    }
}

void hm01b0_read_frame(uint8_t* buffer, size_t length)
{
    struct hm01b0_config* config = &hm01b0_inst.config;

    pio_sm_init(config->pio, config->pio_sm, hm01b0_inst.pio_program_offset, &hm01b0_inst.pio_sm_config);

    int dma_channel = dma_claim_unused_channel(true);

    dma_channel_config dcc = dma_channel_get_default_config(dma_channel);
    channel_config_set_read_increment(&dcc, false);
    channel_config_set_write_increment(&dcc, true);
    channel_config_set_dreq(&dcc, pio_get_dreq(config->pio, config->pio_sm, false));
    channel_config_set_transfer_data_size(&dcc, DMA_SIZE_8);
    
    dma_channel_configure(
        dma_channel,
        &dcc,
        buffer,
        ((uint8_t*)&config->pio->rxf[config->pio_sm]) + 3,
        length,
        false
    );

    dma_channel_start(dma_channel);
    pio_sm_set_enabled(config->pio, config->pio_sm, true);
    pio_sm_put_blocking(config->pio, config->pio_sm, config->width * hm01b0_inst.num_pclk_per_px - 1);
    hm01b0_write_reg8(0x0100, 0x01); // MODE_SELECT
    dma_channel_wait_for_finish_blocking(dma_channel);

    pio_sm_set_enabled(config->pio, config->pio_sm, false);
    dma_channel_unclaim(dma_channel);
    hm01b0_write_reg8(0x0100, 0x00); // MODE_SELECT
}

void hm01b0_set_coarse_integration(unsigned int lines)
{
    if (lines < 2) {
        lines = 2;
    } else if (lines > 0xffff) {
        lines = 0xffff;
    }

    lines -= 2;

    hm01b0_write_reg16(0x0202, lines); // INTEGRATION_H

    hm01b0_write_reg8(0x0104, 0x01); // GRP_PARAM_HOLD
}

static int hm01b0_reset()
{
    hm01b0_write_reg8(0x0103, 0x01);

    for (int retries = 0; retries < 10; retries++) {
        if (hm01b0_read_reg8(0x0100) == 0x00) {
            return 0;
        }

        sleep_ms(100);
    }

    return -1;
}

static uint8_t hm01b0_read_reg8(uint16_t address)
{
    address = __REV16(address);

    uint8_t result = 0xff;

    i2c_write_blocking(hm01b0_inst.config.i2c, HM01B0_I2C_ADDRESS, (const uint8_t*)&address, sizeof(address), false);
    i2c_read_blocking(hm01b0_inst.config.i2c, HM01B0_I2C_ADDRESS, (uint8_t*)&result, sizeof(result), false);

    return result;
}

static uint16_t hm01b0_read_reg16(uint16_t address)
{
    address = __REV16(address);

    uint16_t result = 0xffff;

    i2c_write_blocking(hm01b0_inst.config.i2c, HM01B0_I2C_ADDRESS, (const uint8_t*)&address, sizeof(address), false);
    i2c_read_blocking(hm01b0_inst.config.i2c, HM01B0_I2C_ADDRESS, (uint8_t*)&result, sizeof(result), false);

    return __REV16(result);
}


static void hm01b0_write_reg8(uint16_t address, uint8_t value)
{
    uint8_t data[3];

    *((uint16_t*)data) = __REV16(address);
    data[2] = value;

    i2c_write_blocking(hm01b0_inst.config.i2c, HM01B0_I2C_ADDRESS, data, sizeof(data), false);
}

static void hm01b0_write_reg16(uint16_t address, uint16_t value)
{
    uint8_t data[4];

    *((uint16_t*)data + 0) = __REV16(address);
    *((uint16_t*)data + 1) = __REV16(value);

    i2c_write_blocking(hm01b0_inst.config.i2c, HM01B0_I2C_ADDRESS, data, sizeof(data), false);
}

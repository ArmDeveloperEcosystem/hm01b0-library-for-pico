//
// SPDX-FileCopyrightText: Copyright 2023 Arm Limited and/or its affiliates <open-source-office@arm.com>
// SPDX-License-Identifier: MIT
//

#ifndef _PICO_HM01B0_H_
#define _PICO_HM01B0_H_

#include "hardware/i2c.h"
#include "hardware/pio.h"

struct hm01b0_config {
    i2c_inst_t* i2c;
    uint sda_pin;
    uint scl_pin;

    uint vsync_pin;
    uint hsync_pin;
    uint pclk_pin;

    uint data_pin_base;
    uint data_bits;
    PIO pio;
    uint pio_sm;

    int reset_pin;
    int mclk_pin;

    uint width;
    uint height;
};

int hm01b0_init(const struct hm01b0_config* config);
void hm01b0_deinit();

void hm01b0_read_frame(uint8_t* buffer, size_t length);

void hm01b0_set_coarse_integration(unsigned int lines);

#endif

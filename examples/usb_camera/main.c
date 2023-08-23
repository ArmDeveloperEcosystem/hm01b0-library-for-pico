/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 * Copyright (c) 2023 Arm Limited and/or its affiliates <open-source-office@arm.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "usb_descriptors.h"

#include "pico/hm01b0.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

void video_task(void);

struct hm01b0_config hm01b0_config = {
    .i2c           = PICO_DEFAULT_I2C_INSTANCE,
    .sda_pin       = PICO_DEFAULT_I2C_SDA_PIN,
    .scl_pin       = PICO_DEFAULT_I2C_SCL_PIN,

#ifdef SPARKFUN_MICROMOD
    .vsync_pin     = 25,
    .hsync_pin     = 28,
    .pclk_pin      = 11,
    .data_pin_base = 16,
    .data_bits     = 8,
    .pio           = pio0,
    .pio_sm        = 0,
    .reset_pin     = 24,
    .mclk_pin      = 10,
#else
    .vsync_pin     = 6,
    .hsync_pin     = 7,
    .pclk_pin      = 8,
    .data_pin_base = 9,
    .data_bits     = 1,
    .pio           = pio0,
    .pio_sm        = 0,
    .reset_pin     = -1, // not connected
    .mclk_pin      = -1, // not connected
#endif

    .width         = FRAME_WIDTH,
    .height        = FRAME_HEIGHT,
};

/*------------- MAIN -------------*/
int main(void)
{
  board_init();

#ifdef SPARKFUN_MICROMOD
  gpio_set_dir(25, GPIO_IN);
#endif

  if (hm01b0_init(&hm01b0_config) != 0)
  {
    printf("failed to initialize camera!\n");

    while (1) { tight_loop_contents(); }
  }

  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

  while (1)
  {
    tud_task(); // tinyusb device task

    video_task();
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void) remote_wakeup_en;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
}


//--------------------------------------------------------------------+
// USB Video
//--------------------------------------------------------------------+
static unsigned frame_num = 0;
static unsigned tx_busy = 0;
static unsigned interval_ms = 1000 / FRAME_RATE;

/* YUY2 frame buffer */
static uint8_t frame_buffer[FRAME_WIDTH * FRAME_HEIGHT * 16 / 8];
static uint8_t monochrome_buffer[FRAME_WIDTH * FRAME_HEIGHT];

static void fill_camera_frame(uint8_t *buffer)
{
  const uint8_t* src = monochrome_buffer;

  hm01b0_read_frame(monochrome_buffer, sizeof(monochrome_buffer));

  for (int y = 0; y < FRAME_HEIGHT; y++) {
    for (int x = 0; x < FRAME_WIDTH; x++) {
      *buffer++ = *src++;
      *buffer++ = 128;
    }
  }
}

void video_task(void)
{
  static unsigned start_ms = 0;
  static unsigned already_sent = 0;

  if (!tud_video_n_streaming(0, 0)) {
    already_sent  = 0;
    frame_num     = 0;
    return;
  }

  if (!already_sent) {
    already_sent = 1;
    start_ms = board_millis();
    tx_busy = 1;

    fill_camera_frame(frame_buffer);
    tud_video_n_frame_xfer(0, 0, (void*)frame_buffer, FRAME_WIDTH * FRAME_HEIGHT * 16/8);
  }

  unsigned cur = board_millis();
  if (cur - start_ms < interval_ms) return; // not enough time
  if (tx_busy) return;
  tx_busy = 1;
  start_ms += interval_ms;

  fill_camera_frame(frame_buffer);
  tud_video_n_frame_xfer(0, 0, (void*)frame_buffer, FRAME_WIDTH * FRAME_HEIGHT * 16/8);
}

void tud_video_frame_xfer_complete_cb(uint_fast8_t ctl_idx, uint_fast8_t stm_idx)
{
  (void)ctl_idx; (void)stm_idx;
  tx_busy = 0;
  /* flip buffer */
  ++frame_num;
}

int tud_video_commit_cb(uint_fast8_t ctl_idx, uint_fast8_t stm_idx,
			video_probe_and_commit_control_t const *parameters)
{
  (void)ctl_idx; (void)stm_idx;
  /* convert unit to ms from 100 ns */
  interval_ms = parameters->dwFrameInterval / 10000;
  return VIDEO_ERROR_NONE;
}

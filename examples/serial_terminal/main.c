#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/hm01b0.h"

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

    .width         = 160,
    .height        = 120,
};

// based on: http://paulbourke.net/dataformats/asciiart/
const char REMAP[] = {
    '$', '$', '$', '$', '@', '@', '@', '@', 'B', 'B', 'B', 'B', '%', '%', '%', '8',
    '8', '8', '8', '&', '&', '&', '&', 'W', 'W', 'W', 'M', 'M', 'M', 'M', '#', '#',
    '#', '#', '*', '*', '*', 'o', 'o', 'o', 'o', 'a', 'a', 'a', 'a', 'h', 'h', 'h',
    'h', 'k', 'k', 'k', 'b', 'b', 'b', 'b', 'd', 'd', 'd', 'd', 'p', 'p', 'p', 'q',
    'q', 'q', 'q', 'w', 'w', 'w', 'w', 'm', 'm', 'm', 'Z', 'Z', 'Z', 'Z', 'O', 'O',
    'O', 'O', '0', '0', '0', 'Q', 'Q', 'Q', 'Q', 'L', 'L', 'L', 'L', 'C', 'C', 'C', 
    'C', 'J', 'J', 'J', 'U', 'U', 'U', 'U', 'Y', 'Y', 'Y', 'Y', 'X', 'X', 'X', 'z', 
    'z', 'z', 'z', 'c', 'c', 'c', 'c', 'v', 'v', 'v', 'u', 'u', 'u', 'u', 'n', 'n', 
    'n', 'n', 'x', 'x', 'x', 'x', 'r', 'r', 'r', 'j', 'j', 'j', 'j', 'f', 'f', 'f', 
    'f', 't', 't', 't', '/', '/', '/', '/', '\\', '\\', '\\', '\\', '|', '|', '|', 
    '(', '(', '(', '(', ')', ')', ')', ')', '1', '1', '1', '{', '{', '{', '{', '}',
    '}', '}', '}', '[', '[', '[', '[', ']', ']', ']', '?', '?', '?', '?', '-', '-',
    '-', '-', '_', '_', '_', '+', '+', '+', '+', '~', '~', '~', '~', '<', '<', '<',
    '>', '>', '>', '>', 'i', 'i', 'i', 'i', '!', '!', '!', '!', 'l', 'l', 'l', 'I',
    'I', 'I', 'I', ';', ';', ';', ';', ':', ':', ':', ',', ',', ',', ',', '"', '"',
    '"', '"', '^', '^', '^', '`', '`', '`', '`', '\'', '\'', '\'', '\'', '.', '.', '.', 
    ' '
};

uint8_t pixels[160 * 120];
char row[160 + 1];

int main( void )
{
    // initialize stdio and wait for USB CDC connect
    stdio_init_all();
    while (!stdio_usb_connected()) {
        tight_loop_contents();
    }

    if (hm01b0_init(&hm01b0_config) != 0) {
        printf("failed to initialize camera!\n");

        while (1) { tight_loop_contents(); }
    }

    row[160] = '\0';

    while (true) {
        hm01b0_read_frame(pixels, sizeof(pixels));

        printf("\033[2J");
        for (int y = 0; y < 120; y += 2) {
            printf("\033[%dH", y / 2);
            for (int x = 0; x < 160; x++) {
                uint8_t pixel = pixels[160 * y + x];

                row[x] = REMAP[pixel];
            }
            printf("%s\033[K", row);
        }
        printf("\033[J");
    }
}

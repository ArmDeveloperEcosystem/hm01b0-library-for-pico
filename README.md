# HM01B0 Library for Pico

Capture monochrome images on your [Raspberry Pi Pico](https://www.raspberrypi.com/products/raspberry-pi-pico/) with a [Himax HM01B0 based](https://www.himax.com.tw/products/cmos-image-sensor/always-on-vision-sensors/hm01b0/) camera module.

Learn more in the [Raspberry Pi "Real-time monochrome camera input on Raspberry Pi Pico" guest blog post](https://www.raspberrypi.com/news/real-time-monochrome-camera-input-on-raspberry-pi-pico/).

## Hardware

 * RP2040 board
   * [Raspberry Pi Pico](https://www.raspberrypi.org/products/raspberry-pi-pico/)
   * [Arducam HM01B0 Monochrome QVGA SPI Camera Module for Raspberry Pi Pico](https://www.arducam.com/product/arducam-hm01b0-qvga-spi-camera-module-for-raspberry-pi-pico-2/)
 * SparkFun Micromod
   * [SparkFun MicroMod RP2040 Processor](https://www.sparkfun.com/products/17720)
   * [SparkFun MicroMod Machine Learning Carrier Board](https://www.sparkfun.com/products/16400)
   * [Himax CMOS Imaging Camera - HM01B0](https://www.sparkfun.com/products/15570)

### Default Pinout

| HM01B0 | Raspberry Pi Pico / RP2040 |
| ------ | -------------------------- |
| VCC | 3V3 |
| SCL | GPIO5 |
| SDA | GPIO4 |
| VSYNC | GPIO6 |
| HREF | GPIO7 |
| PCLK | GPIO8 |
| D0 | GPIO9 |
| RESET | - |
| MCLCK | - |
| GND | GND |

GPIO pins are configurable in examples or API.

## Examples

See [examples](examples/) folder.

## Cloning

```sh
git clone https://github.com/ArmDeveloperEcosystem/hm01b0-library-for-pico.git 
```

## Building

1. [Set up the Pico C/C++ SDK](https://datasheets.raspberrypi.org/pico/getting-started-with-pico.pdf)
2. Set `PICO_SDK_PATH`
```sh
export PICO_SDK_PATH=/path/to/pico-sdk
```
3. Create `build` dir, run `cmake` and `make`:
```
mkdir build
cd build
cmake .. -DPICO_BOARD=pico
make
```
For SparkFun MicroMod RP2040 use: `cmake .. -DPICO_BOARD=sparkfun_micromod`

4. Copy example `.uf2` to Pico when in BOOT mode.

## License

[MIT](LICENSE)

## Acknowledgements

The [TinyUSB](https://github.com/hathach/tinyusb) library is used in the `usb_camera` example.

---

Disclaimer: This is not an official Arm product.

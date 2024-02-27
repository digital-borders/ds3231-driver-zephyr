# DS3231 driver for Zephyr

- [Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf)

## Wiring

1. Connect your picoprobe to your pico
2. For the Adafruit DS3231 sensor breakout,

| Pico | Adafruit DS321 breakout |
|------|:-----------------------:|
| GP4  | SDA                     |
| GP5  | SCL                     |
| 3V3  | Vin                     |
| GND  | GND                     |

## Setup

1. Setup a zephyr 3.5 environment on your development machine.
2. Run `west init -m https://github.com/arribada/ds3231-driver-zephyr.git --mr add-time-apis ds3231-env`. You might want to change the branch you want to clone to.
3. Run `west update`

## Flashing

1. Go the app in `examples/simple`
2. To build the example, run `west build -b rpi_pico . -- -DOPENOCD=/usr/bin/openocd -DOPENOCD_DEFAULT_PATH=/usr/share/openocd/scripts -DRPI_PICO_DEBUG_ADAPTER=cmsis-dap`
3. To flash use `west flash`
4. Using a serial utility like `minicom` you can now check the logs. In case of the `examples/shell` app, you can use the same serial utility to interact with the application. Type `help` to get started.

## License

[MIT](./LICENSE)

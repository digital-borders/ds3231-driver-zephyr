# DS3231 driver for Zephyr

- [Datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ds3231.pdf)

## Wiring

1. Connect your picoprobe to your pico
2. For the Adafruit DS3231 sensor breakout,

| Pico | Adafruit DS321 breakout |
|------|:-----------------------:|
| GP2  | SDA                     |
| GP3  | SCL                     |
| 3V3  | Vin                     |
| GND  | GND                     |

## Setup

1. Setup a zephyr 3.5 environment on your development machine.
2. Run `west init -m https://github.com/arribada/ds3231-driver-zephyr.git --mr add-time-apis ds3231-env`. You might want to change the branch you want to clone to.
3. Run `west update`

## Flashing

1. Go the app in `examples/simple`
2. `make` to build the application
3. `make flash` to flash the application
3. `minicom` is configured as the default serial monitor in the Makefile. You can flash and monitor together using `make fm`

## License

[MIT](./LICENSE)

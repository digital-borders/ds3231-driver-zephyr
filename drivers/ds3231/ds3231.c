/*
 * Copyright (c) 2023 Arribada Initiative CIC
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#define DT_DRV_COMPAT analogdevices_ds3231

#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ds3231, CONFIG_RTC_LOG_LEVEL);

/* DS3231 register addresses */
#define DS3231_CONTROL        0x0eU
#define DS3231_CONTROL_STATUS 0x0fU
#define DS3231_AGING_OFFSET   0x10U
#define DS3231_TEMP_MSB       0x11U
#define DS3231_TEMP_LSB       0x12U

/* Time register addresses */
#define DS3231_SECONDS 0x00U
#define DS3231_MINUTES 0x01U
#define DS3231_HOURS   0x02U
#define DS3231_DAYS    0x03U
#define DS3231_DATE    0x04U
#define DS3231_MONTH   0x05U
#define DS3231_YEAR    0x06U

/* Alarm 1 register addresses */
#define DS3231_ALARM_1_SECONDS  0x07U
#define DS3231_ALARM_1_MINUTES  0x08U
#define DS3231_ALARM_1_HOURS    0x09U
#define DS3231_ALARM_1_DAY_DATE 0x0aU

/* Alarm 2 register addresses */
#define DS3231_ALARM_2_MINUTES  0x0bU
#define DS3231_ALARM_2_HOURS    0x0cU
#define DS3231_ALARM_2_DAY_DATE 0x0dU

/* Time and date register bits */
#define DS3231_SECONDS_10              GENMASK(6, 4)
#define DS3231_SECONDS_MASK            GENMASK(3, 0)
#define DS3231_MINUTES_10              GENMASK(6, 4)
#define DS3231_MINUTES_MASK            GENMASK(3, 0)
#define DS3231_HOURS_12_24             BIT(6)
#define DS3231_HOURS_AM_PM_20          BIT(5)
#define DS3231_HOURS_10                BIT(4)
#define DS3231_HOURS_MASK              GENMASK(3, 0)
#define DS3231_DAYS_MASK               GENMASK(2, 0)
#define DS3231_DATE_10                 GENMASK(5, 4)
#define DS3231_DATE_MASK               GENMASK(3, 0)
#define DS3231_MONTH_CENTURY           BIT(7)
#define DS3231_MONTH_10                BIT(4)
#define DS3231_MONTH_MASK              GENMASK(3, 0)
#define DS3231_YEAR_10                 GENMASK(7, 4)
#define DS3231_YEAR_MASK               GENMASK(3, 0)
#define DS3231_ALARM_1_SECONDS_A1M1    BIT(7)
#define DS3231_ALARM_1_SECONDS_10      GENMASK(6, 4)
#define DS3231_ALARM_1_SECONDS_SECONDS GENMASK(3, 0)
#define DS3231_ALARM_1_MINUTES_A1M2    BIT(7)
#define DS3231_ALARM_1_MINUTES_10      GENMASK(6, 4)
#define DS3231_ALARM_1_MINUTES_MINUTES GENMASK(3, 0)
#define DS3231_ALARM_1_HOURS_A1M3      BIT(7)
#define DS3231_ALARM_1_HOURS_12_24     BIT(6)
#define DS3231_ALARM_1_HOURS_AM_PM_20  BIT(5)
#define DS3231_ALARM_1_HOURS_10        BIT(4)
#define DS3231_ALARM_1_HOURS_HOURS     GENMASK(3, 0)
#define DS3231_ALARM_1_DAY_DATE_A1M4   BIT(7)
#define DS3231_ALARM_1_DAY_DATE_DYDT   BIT(6)
#define DS3231_ALARM_1_DAY_DATE_10     GENMASK(5, 4)
#define DS3231_ALARM_1_DAY_DATE_MASK   GENMASK(3, 0)

#define DS3231_ALARM_2_MINUTES_A2M2    BIT(7)
#define DS3231_ALARM_2_MINUTES_10      GENMASK(6, 4)
#define DS3231_ALARM_2_MINUTES_MINUTES GENMASK(3, 0)
#define DS3231_ALARM_2_HOURS_A2M3      BIT(7)
#define DS3231_ALARM_2_HOURS_12_24     BIT(6)
#define DS3231_ALARM_2_HOURS_AM_PM_20  BIT(5)
#define DS3231_ALARM_2_HOURS_10        BIT(4)
#define DS3231_ALARM_2_HOURS_HOURS     GENMASK(3, 0)
#define DS3231_ALARM_2_DAY_DATE_A2M4   BIT(7)
#define DS3231_ALARM_2_DAY_DATE_DYDT   BIT(6)
#define DS3231_ALARM_2_DAY_DATE_10     GENMASK(5, 4)
#define DS3231_ALARM_2_DAY_DATE_MASK   GENMASK(3, 0)

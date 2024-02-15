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
#define DS3231_CONTROL      0x0eU
#define DS3231_STATUS       0x0fU
#define DS3231_AGING_OFFSET 0x10U
#define DS3231_TEMP_MSB     0x11U
#define DS3231_TEMP_LSB     0x12U

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
#define DS3231_MONTHS_MASK              GENMASK(3, 0)
#define DS3231_YEAR_10                 GENMASK(7, 4)
#define DS3231_YEARS_MASK               GENMASK(3, 0)
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

#define DS3231_CONTROL_EOSC  BIT(7)
#define DS3231_CONTROL_BBSQW BIT(6)
#define DS3231_CONTROL_CONV  BIT(5)
#define DS3231_CONTROL_RS2   BIT(4)
#define DS3231_CONTROL_RS1   BIT(3)
#define DS3231_CONTROL_INTCN BIT(2)
#define DS3231_CONTROL_A2IE  BIT(1)
#define DS3231_CONTROL_A1IE  BIT(0)

#define DS3231_STATUS_OSF     BIT(7)
#define DS3231_STATUS_EN32KHZ BIT(3)
#define DS3231_STATUS_BSY     BIT(2)
#define DS3231_STATUS_A2F     BIT(1)
#define DS3231_STATUS_A1F     BIT(0)

#define DS3231_AGING_OFFSET_SIGN BIT(7)
#define DS3231_AGING_OFFSET_DATA GENMASK(6, 0)
#define DS3231_TEMP_MSB_SIGN     BIT(7)
#define DS3231_TEMP_MSB_DATA     GENMASK(6, 0)
#define DS3231_TEMP_LSB_DATA     GENMASK(7, 6)

/* RTC alarm time fields supported by DS3231 */
#define DS3231_RTC_ALARM_1_TIME_MASK                                                               \
	(RTC_ALARM_TIME_MASK_SECOND | RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR |      \
	 RTC_ALARM_TIME_MASK_WEEKDAY | RTC_ALARM_TIME_MASK_MONTHDAY)

#define DS3231_RTC_ALARM_2_TIME_MASK                                                               \
	(RTC_ALARM_TIME_MASK_MINUTE | RTC_ALARM_TIME_MASK_HOUR | RTC_ALARM_TIME_MASK_WEEKDAY |     \
	 RTC_ALARM_TIME_MASK_MONTHDAY)

/* The DS3231 enumerates months 1 to 12, RTC API uses 0 to 11 */
#define DS3231_MONTHS_OFFSET 1

/* The DS3231 only supports two-digit years, calculate offset to use */
#define DS3231_YEARS_OFFSET (2000 - 1900)

struct ds3231_config {
	const struct i2c_dt_spec i2c;
	// todo add interrupt pin, etc.
};
struct ds3231_data {
	struct k_mutex lock;
};

static int ds3231_read_regs(const struct device *dev, uint8_t addr, void *buf, size_t len)
{
	const struct ds3231_config *config = dev->config;
	int err;

	err = i2c_write_read_dt(&config->i2c, &addr, sizeof(addr), buf, len);
	if (err != 0) {
		LOG_ERR("failed to read reg addr 0x%02x, len %d (err %d)", addr, len, err);
		return err;
	}

	return 0;
}

static int ds3231_read_reg8(const struct device *dev, uint8_t addr, uint8_t *val)
{
	return ds3231_read_regs(dev, addr, val, sizeof(*val));
}

static int ds3231_write_regs(const struct device *dev, uint8_t addr, void *buf, size_t len)
{
	const struct ds3231_config *config = dev->config;
	uint8_t block[sizeof(addr) + len];
	int err;

	block[0] = addr;
	memcpy(&block[1], buf, len);

	err = i2c_write_dt(&config->i2c, block, sizeof(block));
	if (err != 0) {
		LOG_ERR("failed to write reg addr 0x%02x, len %d (err %d)", addr, len, err);
		return err;
	}

	return 0;
}

static int ds3231_write_reg8(const struct device *dev, uint8_t addr, uint8_t val)
{
	return ds3231_write_regs(dev, addr, &val, sizeof(val));
}
static int ds3231_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	uint8_t regs[7];
	int err;
	err = ds3231_read_regs(dev,DS3231_SECONDS,&regs, sizeof(regs));
	if (err!=0){
		return err;
	}
	memset(timeptr, 0U, sizeof(*timeptr));
	timeptr->tm_sec = bcd2bin(regs[0] & DS3231_SECONDS_MASK)+bcd2bin(regs[0] & DS3231_SECONDS_10);
	timeptr->tm_min = bcd2bin(regs[1] & DS3231_MINUTES_MASK)+bcd2bin(regs[1] & DS3231_MINUTES_10);
	timeptr->tm_hour = bcd2bin(regs[2] & DS3231_HOURS_MASK)+bcd2bin(regs[2] & DS3231_HOURS_10);
	timeptr->tm_mday = bcd2bin(regs[3] & DS3231_DAYS_MASK);
	timeptr->tm_wday = bcd2bin(regs[4] & DS3231_DATE_MASK);
	timeptr->tm_mon = bcd2bin(regs[5] & DS3231_MONTHS_MASK) - DS3231_MONTHS_OFFSET;
	timeptr->tm_year = bcd2bin(regs[6] & DS3231_YEARS_MASK) + DS3231_YEARS_OFFSET;
	LOG_DBG("get time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
		"min = %d, sec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	return 0;
}

static const struct rtc_driver_api ds3231_driver_api = {
	.get_time = ds3231_get_time,
};

static int ds3231_init(const struct device* dev)
{
	const struct ds3231_config *config = dev->config;
	struct ds3231_data *data = dev->data;
	/* uint8_t regs[3]; */
	/* int err; */
	LOG_INF("Initializing the ds3231 driver");
	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}
	return 0;

}

#define DS3231_INIT(inst)                                                                          \
	static const struct ds3231_config ds3231_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
	};                                                                                         \
	static struct ds3231_data ds3231_data_##inst;                                              \
	DEVICE_DT_INST_DEFINE(inst, &ds3231_init, NULL, &ds3231_data_##inst,                       \
			      &ds3231_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,        \
			      &ds3231_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DS3231_INIT)

/*
 * Copyright (c) 2023 Arribada Initiative CIC
 *
 * SPDX-License-Identifier: MIT
 */
#define DT_DRV_COMPAT adi_ds3231

#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#define PRINTF_BINARY_PATTERN_INT8 "%c%c%c%c%c%c%c%c"
#define PRINTF_BYTE_TO_BINARY_INT8(i)                                                              \
	(((i) & 0x80ll) ? '1' : '0'), (((i) & 0x40ll) ? '1' : '0'), (((i) & 0x20ll) ? '1' : '0'),  \
		(((i) & 0x10ll) ? '1' : '0'), (((i) & 0x08ll) ? '1' : '0'),                        \
		(((i) & 0x04ll) ? '1' : '0'), (((i) & 0x02ll) ? '1' : '0'),                        \
		(((i) & 0x01ll) ? '1' : '0')
#define PRINTF_BINARY_PATTERN_INT16 PRINTF_BINARY_PATTERN_INT8 PRINTF_BINARY_PATTERN_INT8
#define PRINTF_BYTE_TO_BINARY_INT16(i)                                                             \
	PRINTF_BYTE_TO_BINARY_INT8((i) >> 8), PRINTF_BYTE_TO_BINARY_INT8(i)

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
#define DS3231_SECONDS_10                GENMASK(6, 4)
#define DS3231_SECONDS_MASK              GENMASK(3, 0)
#define DS3231_MINUTES_10                GENMASK(6, 4)
#define DS3231_MINUTES_MASK              GENMASK(3, 0)
#define DS3231_HOURS_12_24               BIT(6)
#define DS3231_HOURS_AM_PM_20            BIT(5)
#define DS3231_HOURS_10                  BIT(4)
#define DS3231_HOURS_MASK                GENMASK(3, 0)
#define DS3231_DAYS_MASK                 GENMASK(2, 0)
#define DS3231_DATE_10                   GENMASK(5, 4)
#define DS3231_DATE_MASK                 GENMASK(3, 0)
#define DS3231_MONTH_CENTURY             BIT(7)
#define DS3231_MONTH_10                  BIT(4)
#define DS3231_MONTHS_MASK               GENMASK(3, 0)
#define DS3231_YEAR_10                   GENMASK(7, 4)
#define DS3231_YEARS_MASK                GENMASK(3, 0)
#define DS3231_ALARM_1_SECONDS_A1M1      BIT(7)
#define DS3231_ALARM_1_SECONDS_10        GENMASK(6, 4)
#define DS3231_ALARM_1_SECONDS_SECONDS   GENMASK(3, 0)
#define DS3231_ALARM_1_MINUTES_A1M2      BIT(7)
#define DS3231_ALARM_1_MINUTES_10        GENMASK(6, 4)
#define DS3231_ALARM_1_MINUTES_MINUTES   GENMASK(3, 0)
#define DS3231_ALARM_1_HOURS_A1M3        BIT(7)
#define DS3231_ALARM_1_HOURS_12_24       BIT(6)
#define DS3231_ALARM_1_HOURS_AM_PM_20    BIT(5)
#define DS3231_ALARM_1_HOURS_10          BIT(4)
#define DS3231_ALARM_1_HOURS_HOURS       GENMASK(3, 0)
#define DS3231_ALARM_1_DAY_DATE_A1M4     BIT(7)
#define DS3231_ALARM_1_DAY_DATE_DYDT     BIT(6)
#define DS3231_ALARM_1_DAY_DATE_10       GENMASK(5, 4)
#define DS3231_ALARM_1_DAY_DATE_MASK     GENMASK(3, 0)

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

/* Macro for interrupt pin code */
#if DT_ANY_INST_HAS_PROP_STATUS_OKAY(int1_gpios)
#define DS3231_INT1_GPIOS_IN_USE 1
#endif

struct ds3231_config {
	const struct i2c_dt_spec i2c;

#ifdef DS3231_INT1_GPIOS_IN_USE
	struct gpio_dt_spec int1;
#endif /* DS3231_INT1_GPIOS_IN_USE */
};
struct ds3231_data {
	struct k_mutex lock;
#if DS3231_INT1_GPIOS_IN_USE
	struct gpio_callback int1_callback;
	struct k_thread int1_thread;
	struct k_sem int1_sem;

	K_KERNEL_STACK_MEMBER(int1_stack, CONFIG_RTC_DS3231_THREAD_STACK_SIZE);
#ifdef CONFIG_RTC_ALARM
	rtc_alarm_callback alarm_callback;
	void *alarm_user_data;
#endif /* CONFIG_RTC_ALARM */
#ifdef CONFIG_RTC_UPDATE
	rtc_update_callback update_callback;
	void *update_user_data;
#endif /* CONFIG_RTC_UPDATE */
#endif /* DS3231_INT1_GPIOS_IN_USE */
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

/* static int ds3231_read_reg8(const struct device *dev, uint8_t addr, uint8_t *val) */
/* { */
/*	return ds3231_read_regs(dev, addr, val, sizeof(*val)); */
/* } */

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

/* static int ds3231_write_reg8(const struct device *dev, uint8_t addr, uint8_t val) */
/* { */
/*	return ds3231_write_regs(dev, addr, &val, sizeof(val)); */
/* } */

static int ds3231_set_time(const struct device *dev, const struct rtc_time *timeptr)
{
	int ret = 0;
	uint8_t raw_time[7] = {0};
	raw_time[0] = (bin2bcd(timeptr->tm_sec / 10) << 4) + bin2bcd(timeptr->tm_sec % 10);
	raw_time[1] = (bin2bcd(timeptr->tm_min / 10) << 4) + bin2bcd(timeptr->tm_min % 10);
	raw_time[2] = (bin2bcd(timeptr->tm_hour / 10) << 4) + bin2bcd(timeptr->tm_hour % 10);
	raw_time[3] = bin2bcd(timeptr->tm_wday);
	raw_time[4] = (bin2bcd(timeptr->tm_mday / 10) << 4) + bin2bcd(timeptr->tm_mday % 10);
	raw_time[5] = bin2bcd(timeptr->tm_mon + DS3231_MONTHS_OFFSET) & DS3231_MONTHS_MASK;
	raw_time[6] = bin2bcd(timeptr->tm_year - DS3231_YEARS_OFFSET);

	ret = ds3231_write_regs(dev, DS3231_SECONDS, raw_time, sizeof(raw_time));
	if (ret) {
		LOG_ERR("Error when setting time: %i", ret);
		return ret;
	}

	return 0;
}

static int ds3231_get_time(const struct device *dev, struct rtc_time *timeptr)
{
	uint8_t regs[7];
	int err;
	err = ds3231_read_regs(dev, DS3231_SECONDS, &regs, sizeof(regs));
	if (err != 0) {
		return err;
	}
	memset(timeptr, 0U, sizeof(*timeptr));
	timeptr->tm_sec =
		bcd2bin(regs[0] & DS3231_SECONDS_MASK) + bcd2bin(regs[0] & DS3231_SECONDS_10);
	timeptr->tm_min =
		bcd2bin(regs[1] & DS3231_MINUTES_MASK) + bcd2bin(regs[1] & DS3231_MINUTES_10);
	timeptr->tm_hour =
		bcd2bin(regs[2] & DS3231_HOURS_MASK) + bcd2bin(regs[2] & DS3231_HOURS_10);
	timeptr->tm_wday = bcd2bin(regs[3] & DS3231_DAYS_MASK);
	timeptr->tm_mday = bcd2bin(regs[4] & DS3231_DATE_MASK) + bcd2bin(regs[4] & DS3231_DATE_10);
	timeptr->tm_mon = bcd2bin(regs[5] & DS3231_MONTHS_MASK) +
			  bcd2bin(regs[5] & DS3231_MONTH_10) - DS3231_MONTHS_OFFSET;
	timeptr->tm_year = bcd2bin(regs[6] & DS3231_YEARS_MASK) +
			   bcd2bin(regs[6] & DS3231_YEAR_10) + DS3231_YEARS_OFFSET;
	LOG_DBG("get time: year = %d, mon = %d, mday = %d, wday = %d, hour = %d, "
		"min = %d, sec = %d",
		timeptr->tm_year, timeptr->tm_mon, timeptr->tm_mday, timeptr->tm_wday,
		timeptr->tm_hour, timeptr->tm_min, timeptr->tm_sec);

	return 0;
}
#ifdef CONFIG_RTC_ALARM
static int ds3231_alarm_get_supported_fields(const struct device *dev, uint16_t id, uint16_t *mask)
{

	if (id == 0U) {
		*mask = DS3231_RTC_ALARM_1_TIME_MASK;
	} else if (id == 1U) {
		*mask = DS3231_RTC_ALARM_2_TIME_MASK;
	} else {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}
	LOG_INF("Supported mask is " PRINTF_BINARY_PATTERN_INT16,
		PRINTF_BYTE_TO_BINARY_INT16(*mask));
	return 0;
}

static int ds3231_alarm_get_time(const struct device *dev, uint16_t id, uint16_t *mask,
				 struct rtc_time *timeptr)
{
	int err;
	uint8_t regs_0[4];
	uint8_t regs_1[3];
	if (id == 0u) {
		err = ds3231_read_regs(dev, DS3231_ALARM_1_SECONDS, &regs_0, sizeof(regs_0));
		if (err != 0) {
			return err;
		}
		memset(timeptr, 0U, sizeof(*timeptr));
		timeptr->tm_sec = bcd2bin(regs_0[0] & DS3231_ALARM_1_SECONDS_SECONDS) +
				  bcd2bin(regs_0[0] & DS3231_ALARM_1_SECONDS_10);
		timeptr->tm_min = bcd2bin(regs_0[1] & DS3231_ALARM_1_MINUTES_MINUTES) +
				  bcd2bin(regs_0[1] & DS3231_ALARM_1_MINUTES_10);
		timeptr->tm_hour = bcd2bin(regs_0[2] & DS3231_ALARM_1_HOURS_HOURS) +
				   bcd2bin(regs_0[2] & DS3231_ALARM_1_HOURS_10) +
				   bcd2bin(regs_0[2] & DS3231_ALARM_1_HOURS_AM_PM_20);
		timeptr->tm_mday = bcd2bin(regs_0[3] & DS3231_ALARM_1_DAY_DATE_MASK) +
				   bcd2bin(regs_0[3] & DS3231_ALARM_1_DAY_DATE_10);
	} // endif id=0
	else if (id==1U){
		err = ds3231_read_regs(dev, DS3231_ALARM_2_MINUTES, &regs_1, sizeof(regs_1));
		if (err != 0) {
			return err;
		}
		memset(timeptr, 0U, sizeof(*timeptr));
		timeptr->tm_min = bcd2bin(regs_1[0] & DS3231_ALARM_2_MINUTES_MINUTES) +
				  bcd2bin(regs_1[0] & DS3231_ALARM_2_MINUTES_10);
		timeptr->tm_hour = bcd2bin(regs_1[1] & DS3231_ALARM_2_HOURS_HOURS) +
				   bcd2bin(regs_1[1] & DS3231_ALARM_2_HOURS_10) +
				   bcd2bin(regs_1[1] & DS3231_ALARM_2_HOURS_AM_PM_20);
		timeptr->tm_mday = bcd2bin(regs_1[2] & DS3231_ALARM_2_DAY_DATE_MASK) +
				   bcd2bin(regs_1[2] & DS3231_ALARM_2_DAY_DATE_10);

	}
	return 0;
}

static int ds3231_alarm_is_pending(const struct device *dev, uint16_t id)
{
	return 0;
}

static int ds3231_alarm_set_time(const struct device *dev, uint16_t id, uint16_t mask,
				 const struct rtc_time *timeptr)
{
	uint8_t regs_0[4];
	uint8_t regs_1[3]; // We only need 3 for Alarm 2
	uint8_t reg_INT;
	int ret;
	LOG_INF("Mask is " PRINTF_BINARY_PATTERN_INT16, PRINTF_BYTE_TO_BINARY_INT16(mask));

	if (id == 0U) {
		/* Clear set alarms */
		reg_INT = 0; /* TODO should we only clear the A1F flag here? */
		ret = ds3231_write_regs(dev, DS3231_STATUS, &reg_INT, sizeof(reg_INT));

		if ((mask & ~(DS3231_RTC_ALARM_1_TIME_MASK)) != 0U) {
			LOG_ERR("unsupported alarm field mask 0x%04x", mask);
			return -EINVAL;
		}

		if ((mask & RTC_ALARM_TIME_MASK_SECOND) != 0U) {
			regs_0[0] = (bin2bcd(timeptr->tm_sec / 10) << 4) +
				    bin2bcd(timeptr->tm_sec % 10);

		} else {
			regs_0[0] = DS3231_ALARM_1_SECONDS_A1M1;
		}

		if ((mask & RTC_ALARM_TIME_MASK_MINUTE) != 0U) {
			regs_0[1] = (bin2bcd(timeptr->tm_min / 10) << 4) +
				    bin2bcd(timeptr->tm_min % 10);
		} else {
			regs_0[1] = DS3231_ALARM_1_MINUTES_A1M2;
		}

		if ((mask & RTC_ALARM_TIME_MASK_HOUR) != 0U) {
			regs_0[2] = (bin2bcd(timeptr->tm_hour / 10) << 4) +
				    bin2bcd(timeptr->tm_hour % 10);
		} else {
			regs_0[2] = DS3231_ALARM_1_HOURS_A1M3;
		}

		if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) != 0U) {
			regs_0[3] = (bin2bcd(timeptr->tm_mday / 10) << 4) +
				    bin2bcd(timeptr->tm_mday % 10);
		} else {
			regs_0[3] = DS3231_ALARM_1_DAY_DATE_A1M4;
		}

		ret = ds3231_write_regs(dev, DS3231_ALARM_1_SECONDS, regs_0, sizeof(regs_0));

		// Write bits to enable interrupt generation on alarm 1 (A1E and INTCN)
		reg_INT = DS3231_CONTROL_A1IE + DS3231_CONTROL_INTCN;
		ret = ds3231_write_regs(dev, DS3231_CONTROL, &reg_INT, sizeof(reg_INT));

		return 0;
	} else if (id == 1U) {
		if ((mask & ~(DS3231_RTC_ALARM_2_TIME_MASK)) != 0U) {
			LOG_ERR("unsupported alarm field mask 0x%04x", mask);
			return -EINVAL;
		}
		if ((mask & RTC_ALARM_TIME_MASK_MINUTE) != 0U) {
			regs_1[0] = (bin2bcd(timeptr->tm_min / 10) << 4) +
				    bin2bcd(timeptr->tm_min % 10);
		} else {
			regs_1[0] = DS3231_ALARM_2_MINUTES_A2M2;
		}

		if ((mask & RTC_ALARM_TIME_MASK_HOUR) != 0U) {
			regs_1[1] = (bin2bcd(timeptr->tm_hour / 10) << 4) +
				    bin2bcd(timeptr->tm_hour % 10);
		} else {
			regs_1[1] = DS3231_ALARM_2_HOURS_A2M3;
		}

		if ((mask & RTC_ALARM_TIME_MASK_MONTHDAY) != 0U) {
			regs_1[2] = (bin2bcd(timeptr->tm_mday / 10) << 4) +
				    bin2bcd(timeptr->tm_mday % 10);
		} else {
			regs_1[2] = DS3231_ALARM_2_DAY_DATE_A2M4;
		}

		ret = ds3231_write_regs(dev, DS3231_ALARM_2_MINUTES, regs_1, sizeof(regs_1));

		// Write bits to enable interrupt generation on alarm 2 (A2E and INTCN)
		reg_INT = DS3231_CONTROL_A2IE + DS3231_CONTROL_INTCN;
		ret = ds3231_write_regs(dev, DS3231_CONTROL, &reg_INT, sizeof(reg_INT));

		return 0;
	} else {
		LOG_ERR("invalid ID %d", id);
		return -EINVAL;
	}
}
#if DS3231_INT1_GPIOS_IN_USE
static void ds3231_int1_callback_handler(const struct device *port, struct gpio_callback *cb,
					 gpio_port_pins_t pins)
{
	struct ds3231_data *data = CONTAINER_OF(cb, struct ds3231_data, int1_callback);
	LOG_INF("inside callback handler");
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	k_sem_give(&data->int1_sem);
}

static int ds3231_update_set_callback(const struct device *dev, rtc_update_callback callback,
				      void *user_data)
{
	return 0;
}

static int ds3231_int1_enable(const struct device *dev, bool enable)
{
	int err = 0;
	const struct ds3231_config *config = dev->config;
	err = gpio_pin_interrupt_configure_dt(&config->int1, GPIO_INT_EDGE_TO_ACTIVE);
	return 0;
}

static void ds3231_int1_thread(const struct device *dev)
{
}

static int ds3231_alarm_set_callback(const struct device *dev, uint16_t id,
				     rtc_alarm_callback callback, void *user_data)
{
	const struct ds3231_config *config = dev->config;
	struct ds3231_data *data = dev->data;
	/* uint8_t control_1; */
	int err = 0;

	/* Check if int1 pin is assigned */
	if (config->int1.port == NULL) {
		LOG_INF("int1 port is null!");
		return -ENOTSUP;
	}
	/* Check if valid ID */
	if (id != 0 && id != 1) {
		LOG_ERR("Invalid ID %d - should be 0 or 1", id);
		return -EINVAL;
	}
	/* Enable gpio interrupt settings */
	ds3231_int1_enable(dev, callback != NULL);
	return err;
}
#endif /* DS3231_INT1_GPIOS_IN_USE */
#endif /* CONFIG_RTC_ALARM */

static const struct rtc_driver_api ds3231_driver_api = {
	.set_time = ds3231_set_time,
	.get_time = ds3231_get_time,
#ifdef CONFIG_RTC_ALARM
	.alarm_get_supported_fields = ds3231_alarm_get_supported_fields,
	.alarm_set_time = ds3231_alarm_set_time,
	.alarm_get_time = ds3231_alarm_get_time,
	.alarm_is_pending = ds3231_alarm_is_pending,
#if DS3231_INT1_GPIOS_IN_USE
	.alarm_set_callback = ds3231_alarm_set_callback,
#endif /* DS3231_INT1_GPIOS_IN_USE */
#endif /* CONFIG_RTC_ALARM */

#if DS3231_INT1_GPIOS_IN_USE && defined(CONFIG_RTC_UPDATE)
	.update_set_callback = ds3231_update_set_callback,
#endif /* DS3231_INT1_GPIOS_IN_USE && defined(CONFIG_RTC_UPDATE) */
};

static int ds3231_init(const struct device *dev)
{
	const struct ds3231_config *config = dev->config;
	struct ds3231_data *data = dev->data;
	int err;
	LOG_INF("Initializing the ds3231 driver");

	k_mutex_init(&data->lock);

	if (!i2c_is_ready_dt(&config->i2c)) {
		LOG_ERR("I2C bus not ready");
		return -ENODEV;
	}
#if DS3231_INT1_GPIOS_IN_USE
	k_tid_t tid;

	LOG_INF("Setting up interrupt thread and gpio");

	if (config->int1.port != NULL) {
		k_sem_init(&data->int1_sem, 0, INT_MAX);

		if (!gpio_is_ready_dt(&config->int1)) {
			LOG_ERR("GPIO not ready");
			return -ENODEV;
		}
		LOG_INF("gpio is ready");
		err = gpio_pin_configure_dt(&config->int1, GPIO_INPUT);
		if (err != 0) {
			LOG_ERR("failed to configure GPIO (err %d)", err);
			return -ENODEV;
		}
		LOG_INF("gpio configured as input");
		gpio_init_callback(&data->int1_callback, ds3231_int1_callback_handler,
				   BIT(config->int1.pin));

		err = gpio_add_callback_dt(&config->int1, &data->int1_callback);
		if (err != 0) {
			LOG_ERR("failed to add GPIO callback (err %d)", err);
			return -ENODEV;
		}
		LOG_INF("gpio callback added");
		tid = k_thread_create(&data->int1_thread, data->int1_stack,
				      K_THREAD_STACK_SIZEOF(data->int1_stack),
				      (k_thread_entry_t)ds3231_int1_thread, (void *)dev, NULL, NULL,
				      CONFIG_RTC_DS3231_THREAD_PRIO, 0, K_NO_WAIT);
		k_thread_name_set(tid, "pcf8523");

		/*
		 * Defer GPIO interrupt configuration due to INT1/CLKOUT pin sharing. This allows
		 * using the CLKOUT square-wave signal for RTC calibration when no alarm/update
		 * callbacks are enabled (and not configured as a wakeup-source).
		 */
	}
#endif /* DS3231_INT1_GPIOS_IN_USE */
	return 0;
}

#define DS3231_INIT(inst)                                                                          \
	static const struct ds3231_config ds3231_config_##inst = {                                 \
		.i2c = I2C_DT_SPEC_INST_GET(inst),                                                 \
		 IF_ENABLED(DS3231_INT1_GPIOS_IN_USE,                                              \
			    (.int1 = GPIO_DT_SPEC_INST_GET_OR(inst, int1_gpios, {0})))};           \
												   \
	static struct ds3231_data ds3231_data_##inst;                                              \
	\
	DEVICE_DT_INST_DEFINE(inst, &ds3231_init, NULL, &ds3231_data_##inst,                       \
			      &ds3231_config_##inst, POST_KERNEL, CONFIG_RTC_INIT_PRIORITY,        \
			      &ds3231_driver_api);

DT_INST_FOREACH_STATUS_OKAY(DS3231_INIT)

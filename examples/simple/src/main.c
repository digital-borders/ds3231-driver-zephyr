#include <zephyr/kernel.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <zephyr/sys/timeutil.h>

LOG_MODULE_REGISTER(simple_app);
const struct device *dev;

static const struct device *get_ds3231_device(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(analogdevices_ds3231);

	if (dev == NULL) {
		/* No such node, or the node does not have status "okay". */
		printk("\nError: no device found.\n");
		return NULL;
	}

	if (!device_is_ready(dev)) {
		printk("\nError: Device \"%s\" is not ready; "
		       "check the driver initialization logs for errors.\n",
		       dev->name);
		return NULL;
	}

	printk("Found device \"%s\", getting time\n", dev->name);
	return dev;
}

int main(void)
{
	time_t timer_set = 1708955166;
	struct rtc_time set_t, get_t;

	LOG_INF("Running DS3231 simple  app on %s\n", CONFIG_BOARD);
	dev = get_ds3231_device();
	if (dev == NULL) {
		return 0;
	}

	gmtime_r(&timer_set, (struct tm *)(&set_t));
	LOG_INF("Setting date/time is %d-%d-%d %d  %d:%d:%d\n", set_t.tm_year + 1900,
		set_t.tm_mon + 1, set_t.tm_mday, set_t.tm_wday, set_t.tm_hour, set_t.tm_min,
		set_t.tm_sec);

	int ret = rtc_set_time(dev, &set_t);
	if (ret != 0) {
		LOG_INF("Error setting time!!");
	}

	k_msleep(2000);

	ret = rtc_get_time(dev, &get_t);
	if (ret != 0) {
		LOG_INF("Error getting time!!");
	} else {
		LOG_INF("Date/time is %d-%d-%d   %d:%d:%d\n", get_t.tm_year + 1900,
			get_t.tm_mon + 1, get_t.tm_mday, get_t.tm_hour, get_t.tm_min, get_t.tm_sec);
	}
	return 0;
}

#include <zephyr/kernel.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(simple_app);

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
	LOG_INF("Running DS3231 simple app on %s\n", CONFIG_BOARD);
	const struct device *dev = get_ds3231_device();
	if (dev == NULL) {
		return 0;
	}
	struct rtc_time get_t;
	rtc_get_time(dev, &get_t);
	LOG_INF("Current date/time is %d-%d-%d   %d:%d:%d\n", get_t.tm_year + 1900, get_t.tm_mon + 1, get_t.tm_mday, get_t.tm_hour, get_t.tm_min, get_t.tm_sec);
	return 0;
}

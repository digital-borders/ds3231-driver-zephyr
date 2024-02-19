#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <stdlib.h>
#include <zephyr/sys/timeutil.h>

LOG_MODULE_REGISTER(simple_app);
const struct device *dev;

static int cmd_g_arribada_rtc_set(const struct shell *shell, size_t argc, char *argv[])
{
	time_t timer_set = atoi(argv[1]);
	struct rtc_time set_t;
	gmtime_r(&timer_set, (struct tm *)(&set_t));
	LOG_INF("Setting date/time is %d-%d-%d %d  %d:%d:%d\n", set_t.tm_year + 1900,
		set_t.tm_mon + 1, set_t.tm_mday, set_t.tm_wday, set_t.tm_hour, set_t.tm_min, set_t.tm_sec);

	int ret = rtc_set_time(dev, &set_t);
	if (ret != 0) {
		LOG_INF("Error setting time!!");
	}

	struct rtc_time get_t;
	rtc_get_time(dev, &get_t);
	LOG_INF("Date/time is %d-%d-%d %d  %d:%d:%d\n", get_t.tm_year + 1900,
		get_t.tm_mon + 1, get_t.tm_mday,get_t.tm_wday, get_t.tm_hour, get_t.tm_min, get_t.tm_sec);

	return 0;
}
static int cmd_g_arribada_rtc_get(const struct shell *shell, size_t argc, char *argv[])
{
	struct rtc_time get_t;
	rtc_get_time(dev, &get_t);
	LOG_INF("Date/time is %d-%d-%d   %d:%d:%d\n", get_t.tm_year + 1900,
		get_t.tm_mon + 1, get_t.tm_mday, get_t.tm_hour, get_t.tm_min, get_t.tm_sec);

	return 0;
}

static int cmd_g_arribada_restart(const struct shell *shell, size_t argc, char *argv[])
{
	sys_reboot(SYS_REBOOT_COLD);
	return 0;
}

SHELL_STATIC_SUBCMD_SET_CREATE(g_arribada_cmds,
			       SHELL_CMD_ARG(rtcset, NULL,
					     "set RTC time\n $ arribada rtcset <epoch>\n",
					     cmd_g_arribada_rtc_set, 2, 0),
			       SHELL_CMD_ARG(rtcget, NULL, "get RTC time\n $ arribada rtcget \n",
					     cmd_g_arribada_rtc_get, 1, 0),
			       SHELL_CMD_ARG(restart, NULL,
					     "start app again\n $ arribada  restart\n",
					     cmd_g_arribada_restart, 1, 0),
			       SHELL_SUBCMD_SET_END);

SHELL_CMD_REGISTER(arribada, &g_arribada_cmds, "Manage RTC", NULL);

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
	dev = get_ds3231_device();
	if (dev == NULL) {
		return 0;
	}
	/* struct rtc_time get_t; */
	while (1) {
		/* rtc_get_time(dev, &get_t); */
		/* LOG_INF("Current date/time is %d-%d-%d   %d:%d:%d\n", get_t.tm_year + 1900, */
		/* 	get_t.tm_mon + 1, get_t.tm_mday, get_t.tm_hour, get_t.tm_min, get_t.tm_sec); */
		k_msleep(2000);
	}
	return 0;
}

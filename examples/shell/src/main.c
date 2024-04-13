#include <zephyr/shell/shell.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/rtc.h>
#include <zephyr/sys/printk.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>
#include <stdlib.h>
#include <zephyr/sys/timeutil.h>

LOG_MODULE_REGISTER(shell_app);
const struct device *dev;

static void rtc_alarm_callback_handler(const struct device *dev, uint16_t id,
						void *user_data)
{
	LOG_INF("Alarm ringing!!");
}

static int cmd_g_rtc_alarm_set(const struct shell *shell, size_t argc, char *argv[])
{
	// TODO need to accept alarm mask as input
	time_t timer_set = atoi(argv[2]);
	struct rtc_time alarm_t;
	gmtime_r(&timer_set, (struct tm *)(&alarm_t));
	shell_print(shell,"Setting alarm for %d-%d-%d %d  %d:%d:%d\n", alarm_t.tm_year + 1900,
		alarm_t.tm_mon + 1, alarm_t.tm_mday, alarm_t.tm_wday, alarm_t.tm_hour, alarm_t.tm_min,
		alarm_t.tm_sec);

	int ret, alarm_id;
	/* Print current time in RTC */
	struct rtc_time get_t;
	rtc_get_time(dev, &get_t);
        shell_print(shell,"Current date/time is %d-%d-%d   %d:%d:%d\n", get_t.tm_year + 1900,
		get_t.tm_mon + 1, get_t.tm_mday, get_t.tm_hour, get_t.tm_min, get_t.tm_sec);

	alarm_id = atoi(argv[1]);
	uint16_t alarm_time_mask_supported, alarm_time_mask_set;
	/* Get alarms supported fields */
	ret = rtc_alarm_get_supported_fields(dev, alarm_id, &alarm_time_mask_supported);
	/* Set alarm */
	alarm_time_mask_set = alarm_time_mask_supported;
	ret = rtc_alarm_set_time(dev, 0, alarm_time_mask_set, &alarm_t);

	/* Set callback function for alarm */
        shell_print(shell,"Setting callback alarm function now");
        rtc_alarm_set_callback(dev, alarm_id,
			       rtc_alarm_callback_handler, NULL);
	return 0;
}
static int cmd_g_rtc_alarm_get(const struct shell *shell, size_t argc, char *argv[])
{
	struct rtc_time get_t;
	rtc_alarm_get_time(dev, atoi(argv[1]), 0, &get_t);
	shell_print(shell,"Alarm set for date %d  time%d:%d:%d from now\n", get_t.tm_mday, get_t.tm_hour,
		get_t.tm_min, get_t.tm_sec);      
	return 0;
}
static int cmd_g_rtc_set(const struct shell *shell, size_t argc, char *argv[])
{
	time_t timer_set = atoi(argv[1]);
	struct rtc_time set_t;
	gmtime_r(&timer_set, (struct tm *)(&set_t));
	shell_print(shell,"Setting date/time as %d-%d-%d %d  %d:%d:%d\n", set_t.tm_year + 1900,
		set_t.tm_mon + 1, set_t.tm_mday, set_t.tm_wday, set_t.tm_hour, set_t.tm_min,
		set_t.tm_sec);

	int ret = rtc_set_time(dev, &set_t);
	if (ret != 0) {
		LOG_INF("Error setting time!!");
	}

	struct rtc_time get_t;
	rtc_get_time(dev, &get_t);
        shell_print(shell,"Date/time is %d-%d-%d %d  %d:%d:%d\n", get_t.tm_year + 1900, get_t.tm_mon + 1,
		get_t.tm_mday, get_t.tm_wday, get_t.tm_hour, get_t.tm_min, get_t.tm_sec);

	return 0;
}
static int cmd_g_rtc_get(const struct shell *shell, size_t argc, char *argv[])
{
	struct rtc_time get_t;
	rtc_get_time(dev, &get_t);
        shell_print(shell,"Date/time is %d-%d-%d   %d:%d:%d\n", get_t.tm_year + 1900, get_t.tm_mon + 1,
		get_t.tm_mday, get_t.tm_hour, get_t.tm_min, get_t.tm_sec);

	return 0;
}

SHELL_CMD_ARG_REGISTER(rtc_time_set, NULL, "Set RTC time (epoch)", cmd_g_rtc_set, 2, 0);
SHELL_CMD_ARG_REGISTER(rtc_time_get, NULL, "Get RTC time", cmd_g_rtc_get, 1, 0);
SHELL_CMD_ARG_REGISTER(rtc_alarm_set, NULL, "Set alarm id & time ", cmd_g_rtc_alarm_set, 3, 0);
SHELL_CMD_ARG_REGISTER(rtc_alarm_get, NULL, "Get alarm time set by id", cmd_g_rtc_alarm_get, 2, 0);

static const struct device *get_ds3231_device(void)
{
	const struct device *const dev = DEVICE_DT_GET_ANY(adi_ds3231);

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
	/* Setup gpio forinterrupt */
	
	LOG_INF("Running DS3231 shell app on %s\n", CONFIG_BOARD);
	dev = get_ds3231_device();
	if (dev == NULL) {
		return 0;
	}
	/* struct rtc_time get_t; */
	while (1) {
		/* rtc_get_time(dev, &get_t); */
		/* LOG_INF("Current date/time is %d-%d-%d   %d:%d:%d\n", get_t.tm_year + 1900, */
		/* 	get_t.tm_mon + 1, get_t.tm_mday, get_t.tm_hour, get_t.tm_min, get_t.tm_sec);
		 */
		k_msleep(2000);
	}
	return 0;
}

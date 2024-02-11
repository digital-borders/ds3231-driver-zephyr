zephyr_library()
zephyr_library_sources_ifdef(CONFIG_DS3231 ds3231.c)

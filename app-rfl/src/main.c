#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/regulator.h>

#include <zephyr/drivers/sensor.h>

#include "at_client.h"
#include "zephyr/sys/printk.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define DELAY_MS 300

#define ESP8266_UART DEVICE_DT_GET(DT_NODELABEL(usart2))
#define BARO_SENSOR DEVICE_DT_GET(DT_NODELABEL(bme280))

static const struct device *esp8266 = ESP8266_UART;
static const struct device *baro = BARO_SENSOR;

static struct at_client esp_client;
static char response_buf[1024];

int main(void)
{
	int ret;

	printk("Rocket Flight Logger firmware\n");

	if (!device_is_ready(esp8266)) {
		return 0;
	}

	if (!device_is_ready(baro)) {
		printk("BME280 is not ready.\n");
		return 0;
	}

	k_sleep(K_SECONDS(2));


	ret = at_client_init(&esp_client, esp8266);

	ret = at_client_send_command_sync(&esp_client, "AT", "OK", response_buf, sizeof(response_buf), 1000);

	if (ret < 0) {
		printk("ESP8266 isn't responding\n");
		return 0;
	}

	while (1) {}

	return 0;
}


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

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define DELAY_MS 300

#define ESP_UART DEVICE_DT_GET(DT_NODELABEL(usart2))
#define BME_SENSOR DEVICE_DT_GET(DT_NODELABEL(bmp280))

static const struct device *uart = ESP_UART;
const struct device *esp_en = DEVICE_DT_GET(DT_NODELABEL(esp_en));
const struct device *bme = BME_SENSOR;

static void esp_send(const char *cmd)
{
	for (size_t i = 0; i < strlen(cmd); i++) {
		uart_poll_out(uart, cmd[i]);
	}
}

static void esp_cmd(const char *cmd)
{
	esp_send(cmd);
	esp_send("\r\n");
	k_sleep(K_MSEC(300));
}

static void esp_setup_ap(void)
{
	esp_cmd("AT");
	esp_cmd("ATE0");
	esp_cmd("AT+CWMODE=2");
	esp_cmd("AT+CWSAP=\"Rocket\",\"12345678\",5,3");
	esp_cmd("AT+CIPMUX=1");
	esp_cmd("AT+CIPSERVER=1,80");
}

static void send_page_ap(int link_id)
{
	const char page[] =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n\r\n"
		"<html><body><h1>Hello World!</h1></body></html>";

	char cmd[200];

	snprintf(cmd, sizeof(cmd),
		"AT+CIPSEND=%d,%d", link_id, strlen(page));
	esp_cmd(cmd);
	esp_send(page);
}

static void uart_send_str(const char *s)
{
    while (*s) {
        uart_poll_out(uart, *s++);
    }
}

static int esp_test_at(void)
{
	const char *cmd = "AT\r\n";
	char resp[10];
	size_t pos = 0;

	uart_send_str(cmd);
	// test_uart();

	int64_t deadline = k_uptime_get() + 2000;

	while (k_uptime_get() < deadline) {
		uint8_t c;
		if (uart_poll_in(uart, &c) == 0) {
			if (pos < sizeof(resp) - 1) {
				resp[pos++] = c;
			}

			if (strstr(resp, "OK")) {
				return 0;
			}
		}
	}

	return -1;
}

int main(void)
{
	int rv;

	printk("Zephyr Sandbox App\n");

	if (!uart) {
		return -1;
	}

	if (!device_is_ready(bme)) {
		printk("BME280 is not ready.\n");
		return 0;
	}

	// test_uart();

	// regulator_disable(esp_en);
	// k_sleep(K_SECONDS(3));

	rv = esp_test_at();

	if (rv == 0) {
		printk("ESP8266 responded OK\n");
    } else {
        printk("ESP8266 did not respond\n");
    }

	esp_setup_ap();

	char buf[256];
	int pos = 0;

	struct sensor_value val_temperature, val_humidity, val_pressure;

	while (1) {
		// uint8_t c;
		// if (uart_poll_in(uart, &c) == 0) {
		// 	if (pos < sizeof(buf) - 1) {
		// 		buf[pos++] = c;
		// 		// buf[pos] = 0;
		// 	}

		// 	if (strstr(buf, "+DIST_STA_IP:")) {
		// 		send_page_ap(0);
		// 		pos = 0;
		// 	}
		// }

		rv = sensor_sample_fetch(bme);
		if (rv) {
			printk("sensor_sample_fetch failed ret %d\n", rv);
			return 0;
		}

		rv = sensor_channel_get(bme, SENSOR_CHAN_AMBIENT_TEMP, &val_temperature);
		rv = sensor_channel_get(bme, SENSOR_CHAN_PRESS, &val_pressure);
		rv = sensor_channel_get(bme, SENSOR_CHAN_HUMIDITY, &val_humidity);

		double temp = sensor_value_to_double(&val_temperature);
		double press = sensor_value_to_double(&val_pressure);
		double hum = sensor_value_to_double(&val_humidity);

		printf("Temp = %f\r\nHum = %f\r\nPress = %f\r\n",
			temp,
			hum,
			press);
		k_sleep(K_MSEC(500));
	}

	return 0;
}


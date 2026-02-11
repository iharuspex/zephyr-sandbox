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

#define ESP8266_UART DEVICE_DT_GET(DT_NODELABEL(usart2))
#define BARO_SENSOR DEVICE_DT_GET(DT_NODELABEL(bme280))

static const struct device *esp8266 = ESP8266_UART;
static const struct device *baro = BARO_SENSOR;

static void esp_send(const char *cmd)
{
	for (size_t i = 0; i < strlen(cmd); i++) {
		uart_poll_out(esp8266, cmd[i]);
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
        uart_poll_out(esp8266, *s++);
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
		if (uart_poll_in(esp8266, &c) == 0) {
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

	if (!device_is_ready(esp8266)) {
		return -1;
	}

	if (!device_is_ready(baro)) {
		printk("BME280 is not ready.\n");
		return 0;
	}

	k_sleep(K_SECONDS(2));

	rv = esp_test_at();

	if (rv == 0) {
		printk("ESP8266 responded OK\n");
    } else {
        printk("ESP8266 did not respond\n");
    }

	esp_setup_ap();

	char buf[256];
	int pos = 0;

	while (1) {
		uint8_t c;
		if (uart_poll_in(esp8266, &c) == 0) {
			if (pos < sizeof(buf) - 1) {
				buf[pos++] = c;
				// buf[pos] = 0;
			}

			if (strstr(buf, "+DIST_STA_IP:")) {
				send_page_ap(0);
				pos = 0;
			}
		}
	}

	return 0;
}


#include <stddef.h>
#include <stdio.h>

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/regulator.h>

#define DELAY_MS 300

#define ESP_UART DEVICE_DT_GET(DT_NODELABEL(usart2))

static const struct device *uart;
const struct device *esp_en = DEVICE_DT_GET(DT_NODELABEL(esp_en));

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

int main(void)
{
	uart = ESP_UART;

	if (!uart) {
		return -1;
	}

	regulator_disable(esp_en);
	k_sleep(K_SECONDS(2));

	esp_test();

	esp_setup_ap();

	char buf[256];
	int pos = 0;

	while (1) {
		uint8_t c;
		if (uart_poll_in(uart, &c) == 0) {
			if (pos < sizeof(buf) - 1) {
				buf[pos++] = c;
				buf[pos] = 0;
			}

			if (strstr(buf, "+IPD,")) {
				send_page_ap(0);
				pos = 0;
			}
		}
	}

	return 0;
}


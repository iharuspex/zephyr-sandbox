#include "zephyr/net/http/client.h"
#include "zephyr/net/wifi.h"
#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/device.h>

#include <zephyr/net/wifi_mgmt.h>
#include <zephyr/net/net_if.h>
#include <zephyr/net/net_event.h>
#include <zephyr/net/http/server.h>

static void wifi_ap_start(void)
{
	struct net_if *iface = net_if_get_default();

	struct wifi_connect_req_params params = {
		.ssid = "ESP32_AP",
		.ssid_length = strlen("ESP32_AP"),
		.psk = "12345678",
		.psk_length = strlen("12345678"),
		.channel = 6,
		.security = WIFI_SECURITY_TYPE_PSK,
	};

	int ret = net_mgmt(NET_REQUEST_WIFI_AP_ENABLE, iface, &params, sizeof(params));

	if (ret) {
		printk("AP start failed: %d\n", ret);
	} else {
		printk("AP started: SSID=ESP32_AP\n");
	}
}

static int hello_handler(struct http_request *req, struct http_response *rsp, void *user_data)
{
	static const char html[] =
		"HTTP/1.1 200 OK\r\n"
		"Content-Type: text/html\r\n"
		"Connection: close\r\n"
		"\r\n"
		"<html>"
		"<head><title>Hello</title></head>"
		"<body><h1>Hello World</h1></body>"
		"</html>";

	return 0;
}





static void server_start(void)
{
	int ret = http_server_start();
	if (ret) {
		printk("HTTP server start failed: %d\n", ret);
	} else {
		printk("HTTP server started\n");
	}
}

int main(void)
{
	printk("ESP32-C3 AP + HTTP example\n");

	wifi_ap_start();
	server_start();

	return 0;
}


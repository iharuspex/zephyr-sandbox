#include "zephyr/bluetooth/conn.h"
#include <stdint.h>
#include <stdio.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/gatt.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <zephyr/device.h>

#include "lsm303_ll.h"
#include "zephyr/sys/printk.h"

static struct bt_conn *current_conn;

uint32_t stepcount_value = 0;

static void connected(struct bt_conn *conn, uint8_t err)
{

}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{

}

static struct bt_conn_cb conn_callbacks = {
	.connected = connected,
	.disconnected = disconnected,
};

int main(void) {
	int err;
	int old_stepcount = 0;
	err = lsm303_ll_begin();
	if (err < 0) {
		printk("\nError initializing lsm303.  Error code = %d\n",err);
		return 1;
	}

	if (lsm303_countSteps(&stepcount_value) < 0)
	{
		printk("Error starting step counter\n");
		while(1);
	}

	// err = bt_enable(NULL);
	// bt_conn_cb_register(&conn_callbacks);

	while (1) {
		k_msleep(100);

		if (stepcount_value != old_stepcount) {
			old_stepcount = stepcount_value;
			printk("Steps: %d\n", stepcount_value);
		}
	}

	return 0;
}

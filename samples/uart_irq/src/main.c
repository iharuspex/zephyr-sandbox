#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#define MSG_SIZE 32

K_MSGQ_DEFINE(uart_msgq, MSG_SIZE, 10, 4);

static const struct device *usart3 = DEVICE_DT_GET(DT_NODELABEL(usart3));

static char rx_buf[MSG_SIZE];
static int rx_buf_pos;

void uart_callback(const struct device *dev, void *user_data)
{
	uint8_t c;

	if (!uart_irq_update(dev)) {
		return;
	}

	if (!uart_irq_rx_ready(dev)) {
		return;
	}

	while (uart_fifo_read(dev, &c, 1) == 1) {
		if ((c == '\n' || c == '\r') && rx_buf_pos > 0) {
			rx_buf[rx_buf_pos] = '\0';

			k_msgq_put(&uart_msgq, &rx_buf, K_NO_WAIT);

			rx_buf_pos = 0;
		} else if (rx_buf_pos < (sizeof(rx_buf) - 1)) {
			rx_buf[rx_buf_pos++] = c;
		}
	}
}

void print_uart(char *buf)
{
	int msg_len = strlen(buf);

	for (int i = 0; i < msg_len; i++) {
		uart_poll_out(usart3, buf[i]);
	}
}

int main(void)
{
	int ret;
	char tx_buf[MSG_SIZE];

	printk("Async UART sample\n");

	if (!device_is_ready(usart3)) {
		printk("UART device isn't ready");
		return 0;
	}

	ret = uart_irq_callback_user_data_set(usart3, uart_callback, NULL);

	if (ret < 0) {
		if (ret == -ENOTSUP) {
			printk("Interrupt-driven UART API support isn't enabled\n");
		} else if (ret == -ENOSYS) {
			printk("UART device doesn't support interrupt-driven API\n");
		} else {
			printk("Error setting UART callback: %d\n", ret);
		}
		return 0;
	}
	uart_irq_rx_enable(usart3);

	while (k_msgq_get(&uart_msgq, &tx_buf, K_FOREVER) == 0) {
		printk("Received: %s\n", tx_buf);
		print_uart(tx_buf);
	}

	return 0;
}


#include "at_client.h"
#include "syscalls/uart.h"
#include "zephyr/kernel.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(at_client, CONFIG_LOG_DEFAULT_LEVEL);

static void uart_cb(const struct device *dev, void *user_data)
{
    struct at_client *client = (struct at_client *)user_data;

    while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        if (uart_irq_rx_ready(dev)) {
            uint8_t data;
            while (uart_fifo_read(dev, &data, 1) == 1) {
                ring_buf_put(&client->rx_ringbuf, &data, 1);
            }
            k_sem_give(&client->rx_sem);
        }
    }
}

static bool is_urc(const char *line)
{
    if (line[0] == '+') {
        return true;
    }

    if (strcmp(line, "OK") == 0 ||
        strcmp(line, "ERROR") == 0 ||
        strncmp(line, "CONNECT", 7) == 0 ||
        strncmp(line, "NO CARRIER", 10) == 0 ||
        strncmp(line, "BUSY", 4) == 0 ||
        strncmp(line, "NO DIALTONE", 11) == 0 ||
        strncmp(line, "RING", 4) == 0)
    {
        return false;
    }

    return true;
}

static void process_line(struct at_client *client, const char *line)
{
    LOG_DBG("RX line: %s", line);

    k_mutex_lock(&client->lock, K_FOREVER);

    if (client->pending.is_pending) {
        if (strcmp(line, client->pending.expected_response) == 0 ||
            strcmp(line, "ERROR") == 0)
        {
            LOG_DBG("Command completed with: %s", line);

            if (client->pending.response_buffer) {
                snprintf(client->pending.response_buffer + client->pending.response_len,
                    AT_CLIENT_MAX_REPONSE_LEN - client->pending.response_len,
                    "%s\r\n", line);
            }

            if (client->pending.callback) {
                client->pending.callback(client->pending.response_buffer ?
                                        client->pending.response_buffer : line,
                                        client->pending.user_data);
            }

            client->pending.is_pending = false;
            client->pending.response_buffer = NULL;
        } else {
            LOG_DBG("Intermediate response: %s", line);

            if (client->pending.response_buffer) {
                size_t len = strlen(line);
                if (client->pending.response_len + len + 3 < AT_CLIENT_MAX_REPONSE_LEN) {
                    memcpy(client->pending.response_buffer + client->pending.response_len, line, len);

                    client->pending.response_len += len;
                    client->pending.response_buffer[client->pending.response_len++] = '\r';
                    client->pending.response_buffer[client->pending.response_len++] = '\n';
                    client->pending.response_buffer[client->pending.response_len++] = '\0';
                }
            }
        }
    } else {
        if (is_urc(line) && client->urc_callback) {
            LOG_DBG("URC received: %s", line);
            client->urc_callback(line, client->urc_user_data);
        }
    }

    k_mutex_unlock(&client->lock);
}

static void rx_work_handler(struct k_work *work)
{
    struct at_client *client = CONTAINER_OF(work, struct at_client, rx_work);
    uint8_t data;

    while (k_sem_take(&client->rx_sem, K_NO_WAIT) == 0) {
        while (ring_buf_get(&client->rx_ringbuf, &data, 1) == 1) {
            if (data == '\r' || data == '\n') {
                if (client->rx_line_len > 0) {
                    client->rx_line[client->rx_line_len] = '\0';
                    process_line(client, client->rx_line);
                    client->rx_line_len = 0;
                }
            } else if (client->rx_line_len < AT_CLIENT_MAX_REPONSE_LEN - 1) {
                // ignore control characters
                if (data >= 32 && data <= 126) {
                    client->rx_line[client->rx_line_len++] = data;
                }
            }
        }
    }
}

static void timeout_work_handler(struct k_work *work)
{
    struct at_client *client = CONTAINER_OF(work, struct at_client, rx_work);

    k_mutex_lock(&client->lock, K_FOREVER);

    if (client->pending.is_pending) {
        uint32_t now = k_uptime_get_32();
        if (now - client->pending.start_time > AT_CLIENT_CMD_TIMEOUT) {
            LOG_ERR("Command timeout");

            if (client->pending.callback) {
                client->pending.callback("TIMEOUT", client->pending.user_data);
            }

            client->pending.is_pending = false;
            client->pending.response_buffer = NULL;
        } else {
            k_work_reschedule(&client->timeout_work, K_MSEC(100));
        }
    }

    k_mutex_unlock(&client->lock);
}

int at_client_init(struct at_client *client, const struct device *uart_dev)
{
    if (!client || !uart_dev) {
        return -EINVAL;
    }

    memset(client, 0, sizeof(*client));

    client->uart_dev = uart_dev;

    ring_buf_init(&client->rx_ringbuf, sizeof(client->rx_buf), client->rx_buf);

    k_sem_init(&client->rx_sem, 0, 1);
    k_mutex_init(&client->lock);

    k_work_init(&client->rx_work, rx_work_handler);
    k_work_init_delayable(&client->timeout_work, timeout_work_handler);

    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART is not ready");
        return -ENODEV;
    }

    int ret = uart_irq_callback_user_data_set(uart_dev, uart_cb, client);
    if (ret < 0) {
        LOG_ERR("Failed to set UART callback: %d", ret);
        return ret;
    }

    uart_irq_rx_enable(uart_dev);

    client->initialized = true;

    LOG_INF("AT Client initialized");
    return 0;
}

int at_client_send_command(struct at_client *client,
                        const char *command,
                        const char *expected_response,
                        at_response_callback_t callback,
                        void *user_data,
                        uint32_t timeout_ms)
{
    if (!client || !command || !expected_response) {
        return -EINVAL;
    }

    k_mutex_lock(&client->lock, K_FOREVER);

    if (client->pending.is_pending) {
        k_mutex_unlock(&client->lock);
        LOG_WRN("Command pending, can't send new command");
        return -EBUSY;
    }

    size_t cmd_len = strlen(command);
    int ret = uart_tx(client->uart_dev, (const uint8_t *)command, cmd_len, SYS_FOREVER_MS);
    if (ret < 0) {
        k_mutex_unlock(&client->lock);
        LOG_ERR("Failed to send command: %d", ret);
        return ret;
    }

    ret = uart_tx(client->uart_dev, (const uint8_t *)"\r\n", 2, SYS_FOREVER_MS);
    if (ret < 0) {
        k_mutex_unlock(&client->lock);
        LOG_ERR("Failed to send CRLF: %d", ret);
        return ret;
    }

    LOG_DBG("Command sent: %s", command);

    client->pending.expected_response = expected_response;
    client->pending.callback = callback;
    client->pending.user_data = user_data;
    client->pending.start_time = k_uptime_get_32();
    client->pending.is_pending = true;
    client->pending.response_buffer = NULL;
    client->pending.response_len = 0;

    k_work_reschedule(&client->timeout_work, K_MSEC(timeout_ms));

    k_mutex_unlock(&client->lock);

    return 0;
}

static void sync_callback(const char *response, void *user_data) {
    struct k_sem *sem = (struct k_sem *)user_data;
    k_sem_give(sem);
}

static int send_uart(const struct device *uart_dev, const uint8_t *command, size_t cmd_len) {
    for (int i = 0; i < cmd_len; i++) {
        uart_poll_out(uart_dev, command[i]);
    }

    return 0;
}

int at_client_send_command_sync(struct at_client *client,
                            const char *command,
                            const char *expected_response,
                            char *response_buf,
                            size_t response_buf_size,
                            uint32_t timeout_ms)
{
    if (!client || !command || !expected_response || !response_buf) {
        return -EINVAL;
    }

    struct k_sem sync_sem;
    int ret;

    k_sem_init(&sync_sem, 0, 1);
    response_buf[0] = '\0';

    k_mutex_lock(&client->lock, K_FOREVER);

    if (client->pending.is_pending) {
        k_mutex_unlock(&client->lock);
        return -EBUSY;
    }

    size_t cmd_len = strlen(command);
    // ret = uart_tx(client->uart_dev, (const uint8_t *)command, cmd_len, SYS_FOREVER_MS);
    ret = send_uart(client->uart_dev, (const uint8_t *)command, cmd_len);
    if (ret < 0) {
        k_mutex_unlock(&client->lock);
        LOG_ERR("Failed to send command: %d", ret);
        return ret;
    }

    // ret = uart_tx(client->uart_dev, (const uint8_t *)"\r\n", 2, SYS_FOREVER_MS);
    ret = send_uart(client->uart_dev, (const uint8_t *)"\r\n", 2);
    k_work_submit(&client->rx_work);
    if (ret < 0) {
        k_mutex_unlock(&client->lock);
        LOG_ERR("Failed to send CRLF: %d", ret);
        return ret;
    }

    client->pending.expected_response = expected_response;
    client->pending.callback = sync_callback;
    client->pending.user_data = &sync_sem;
    client->pending.start_time = k_uptime_get_32();
    client->pending.is_pending = true;
    client->pending.response_buffer = response_buf;
    client->pending.response_len = 0;
    client->pending.response_buffer[0] = '\0';

    k_mutex_unlock(&client->lock);

    ret = k_sem_take(&sync_sem, K_MSEC(timeout_ms));

    if (ret < 0) {
        k_mutex_lock(&client->lock, K_FOREVER);
        client->pending.is_pending = false;
        client->pending.response_buffer = NULL;
        k_mutex_unlock(&client->lock);

        return -ETIMEDOUT;
    }

    return 0;
}

void at_client_set_urc_callback(struct at_client *client,
                            at_response_callback_t callback,
                            void *user_data)
{
    if (client) {
        k_mutex_lock(&client->lock, K_FOREVER);
        client->urc_callback = callback;
        client->urc_user_data = user_data;
        k_mutex_unlock(&client->lock);
    }
}

void at_client_cancel_pending(struct at_client *client)
{
    if (client) {
        k_mutex_lock(&client->lock, K_FOREVER);

        if (client->pending.is_pending) {
            LOG_WRN("Cancelling pending command");
            client->pending.is_pending = false;
            client->pending.response_buffer = NULL;
            k_work_cancel_delayable(&client->timeout_work);
        }
        k_mutex_unlock(&client->lock);
    }
}

void at_client_deinit(struct at_client *client)
{
    if (!client || !client->initialized) {
        return;
    }

    uart_irq_rx_disable(client->uart_dev);
    uart_irq_tx_disable(client->uart_dev);

    client->initialized = false;

    LOG_INF("AT Client deinitialized");
}
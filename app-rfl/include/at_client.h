#ifndef AT_CLIENT_H_
#define AT_CLIENT_H_

#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define AT_CLIENT_MAX_REPONSE_LEN 128
#define AT_CLIENT_RX_RING_BUF_SIZE 256
#define AT_CLIENT_CMD_TIMEOUT 5000
#define AT_CLIENT_MAX_COMMANDS 10

typedef void (*at_response_callback_t) (const char *response, void *user_data);

struct pending_command {
    const char *expected_response;
    at_response_callback_t callback;
    void *user_data;
    uint32_t start_time;
    bool is_pending;
    char *response_buffer;
    size_t response_len;
};

struct at_client {
    const struct device *uart_dev;
    struct ring_buf rx_ringbuf;
    uint8_t rx_buf[AT_CLIENT_RX_RING_BUF_SIZE];

    char rx_line[AT_CLIENT_MAX_REPONSE_LEN];
    size_t rx_line_len;

    struct k_work rx_work;
    struct k_work_delayable timeout_work;
    struct k_sem rx_sem;
    struct k_mutex lock;

    struct pending_command pending;

    at_response_callback_t urc_callback;
    void *urc_user_data;

    bool initialized;
};

int at_client_init(struct at_client *client, const struct device *uart_dev);

int at_client_send_command(struct at_client *client,
                        const char *command,
                        const char *expected_response,
                        at_response_callback_t callback,
                        void *user_data,
                        uint32_t timeout_ms);

int at_client_send_command_sync(struct at_client *client,
                            const char *command,
                            const char *expected_response,
                            char *response_buf,
                            size_t response_buf_size,
                            uint32_t timeout_ms);

void at_client_set_urc_callback(struct at_client *client,
                            at_response_callback_t callback,
                            void *user_data);

void at_client_cancel_pending(struct at_client *client);

void at_client_deinit(struct at_client *client);

#endif /* AT_CLIENT_H_ */
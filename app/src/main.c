#include <stdio.h>

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include <zephyr/drivers/pwm.h>
#include <zephyr/device.h>

#define DELAY_MS 300

#define PERIOD_MAX PWM_USEC(3900)
#define BEEP_DURATION  K_MSEC(60)

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static const struct pwm_dt_spec pwm = PWM_DT_SPEC_GET(DT_PATH(zephyr_user));

int main(void)
{
	int ret;
	bool led_state = true;

	uint32_t period = PWM_USEC(2000);

	printk("Zephyr Sandbox App\n");

	if (!gpio_is_ready_dt(&led)) {
		return 0;
	}

	if (!pwm_is_ready_dt(&pwm)) {
		printk("%s: device is not ready\n", pwm.dev->name);
		return 0;
	}

	pwm_set_dt(&pwm, period, period / 2U);
	k_sleep(BEEP_DURATION);
	pwm_set_pulse_dt(&pwm, 0);
	k_sleep(K_MSEC(50));

	ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
	if (ret < 0) {
		return 0;
	}

	while (1) {
		ret = gpio_pin_toggle_dt(&led);
		if (ret < 0) {
			return 0;
		}

		led_state = !led_state;
		printf("LED state: %s\n", led_state ? "ON" : "OFF");
		k_msleep(DELAY_MS);
	}

	return 0;
}


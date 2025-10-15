/*
 * Copyright (c) 2019 Manivannan Sadhasivam
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/device.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/sys/util.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <inttypes.h>
#include <errno.h>

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)

BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DEFAULT_RADIO_NODE),
			"No default LoRa radio specified in DT");

#define MAX_DATA_LEN 12
#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL

/*
 * Get button configuration from the devicetree sw0 alias. This is mandatory.
 */
#define SW0_NODE	DT_ALIAS(sw1)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define SLEEP_TIME_MS 1

static struct gpio_callback send_button_cb_data;

static const struct gpio_dt_spec send_button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_dt_spec send_led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0});

LOG_MODULE_REGISTER(main);

char data[MAX_DATA_LEN] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd', ' ', '0'};

const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
struct lora_modem_config config;
// const struct k_poll_signal 

void send_button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	int ret;
	// LOG_INF("Send button pressed at %" PRIu32 "\n", k_cycle_get_32());
	ret = lora_send_async(lora_dev, data, MAX_DATA_LEN, NULL);	// No event signalling yet, thus NULL
	if (ret < 0) {
		LOG_ERR("LoRa send failed");
		return 0;
	}

	gpio_pin_toggle_dt(&send_led);

	// LOG_INF("Data sent %c!", data[MAX_DATA_LEN - 1]);
}

int main(void)
{
	int ret;
	if (!gpio_is_ready_dt(&send_button)) {
		LOG_ERR("Error: button device %s is not ready",
		       send_button.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&send_button, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d",
		       ret, send_button.port->name, send_button.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&send_button,
					      GPIO_INT_EDGE_TO_ACTIVE);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d",
			ret, send_button.port->name, send_button.pin);
		return 0;
	}

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return 0;
	}

	config.frequency = 865100000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_8;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx_power = 14;
	config.tx = true;

	ret = lora_config(lora_dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa config failed");
		return 0;
	}

	gpio_init_callback(&send_button_cb_data, send_button_pressed, BIT(send_button.pin));
	gpio_add_callback(send_button.port, &send_button_cb_data);
	LOG_INF("Set up button at %s pin %d", send_button.port->name, send_button.pin);

	if (send_led.port && !gpio_is_ready_dt(&send_led)) {
		LOG_ERR("Error %d: LED device %s is not ready; ignoring it", ret, send_led.port->name);
		send_led.port = NULL;
	}
	if (send_led.port) {
		ret = gpio_pin_configure_dt(&send_led, GPIO_OUTPUT);
		if (ret != 0) {
			LOG_ERR("Error %d: failed to configure LED device %s pin %d", ret, send_led.port->name, send_led.pin);
			send_led.port = NULL;
		} else {
			LOG_INF("Set up LED at %s pin %d", send_led.port->name, send_led.pin);
		}
	}

	LOG_INF("Press button 0 to send a LoRa packet");
	while (1) {

		int val = gpio_pin_get_dt(&send_button);

		if (val >= 0) {
			gpio_pin_set_dt(&send_led, val);
		}
		k_msleep(SLEEP_TIME_MS);
		// ret = lora_send(lora_dev, data, MAX_DATA_LEN);
		// ret = lora_send_async(lora_dev, data, MAX_DATA_LEN, NULL);	// No event signalling yet, thus NULL
		// if (ret < 0) {
		// 	LOG_ERR("LoRa send failed");
		// 	return 0;
		// }

		// LOG_INF("Data sent %c!", data[MAX_DATA_LEN - 1]);

		/* Send data at 1s interval */
		// k_sleep(K_MSEC(10000));

		/* Increment final character to differentiate packets */
	// 	if (data[MAX_DATA_LEN - 1] == '9') {
	// 		data[MAX_DATA_LEN - 1] = '0';
	// 	} else {
	// 		data[MAX_DATA_LEN - 1] += 1;
	// 	}
	}
	return 0;
}

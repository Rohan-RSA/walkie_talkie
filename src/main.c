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
#define SW0_NODE	DT_ALIAS(sw0)
#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define SLEEP_TIME_MS 100
/* size of stack area used by each thread */
#define STACKSIZE 4096
/* scheduling priority used by each thread */
#define PRIORITY 7

static struct gpio_callback send_button_cb_data;

static const struct gpio_dt_spec send_button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_dt_spec send_led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0});

LOG_MODULE_REGISTER(main);

char data[MAX_DATA_LEN] = {'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r', 'l', 'd', ' ', '0'};

const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
struct lora_modem_config config;

volatile bool boot = true;

static struct k_thread transmit_thread_id;
void transmit_thread(void *dummy1, void *dummy2, void *dummy3)
{
	ARG_UNUSED(dummy1);
	ARG_UNUSED(dummy2);
	ARG_UNUSED(dummy3);

	int ret;

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
	
	while (1)
	{
		LOG_INF("Transmitting");
		ret = lora_send(lora_dev, data, MAX_DATA_LEN);
		if (ret < 0) {
			LOG_ERR("LoRa send failed");
			return 0;
		}

		/* Switch back to receive mode */
		config.frequency = 865100000;
		config.bandwidth = BW_125_KHZ;
		config.datarate = SF_8;
		config.preamble_len = 8;
		config.coding_rate = CR_4_5;
		config.iq_inverted = false;
		config.public_network = false;
		config.tx_power = 14;
		config.tx = false;
		ret = lora_config(lora_dev, &config);
		if (ret < 0) {
			LOG_ERR("LoRa config failed");
			return 0;
		}

		k_thread_suspend(&transmit_thread_id);
	}
}
K_THREAD_STACK_DEFINE(transmit_stack_area, STACKSIZE);

// static struct k_thread receive_thread_id;
// void receive_thread(void *dummy1, void *dummy2, void *dummy3)
// {
// 	ARG_UNUSED(dummy1);
// 	ARG_UNUSED(dummy2);
// 	ARG_UNUSED(dummy3);
// }
// K_THREAD_STACK_DEFINE(receive_stack_area, STACKSIZE);

void send_button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	// LOG_INF("Send button pressed at %" PRIu32 "\n", k_cycle_get_32());
	gpio_pin_toggle_dt(&send_led);

	if(boot) {
		boot = false;
		k_thread_start(&transmit_thread_id);
	}
	else k_thread_resume(&transmit_thread_id);
}

void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
		     int16_t rssi, int8_t snr, void *user_data)
{
	static int cnt;

	// ARG_UNUSED(dev);
	// ARG_UNUSED(size);
	// ARG_UNUSED(user_data);
	LOG_INF("Device %s", dev->name);
	LOG_INF("LoRa RX RSSI: %d dBm, SNR: %d dB", rssi, snr);
	// LOG_HEXDUMP_INF(data, size, "LoRa RX payload");
	LOG_INF("Data: %s\r\nSize: %d\r\nrssi: %d\r\nsnr: %d", data, size, rssi, snr);

	/* Stop receiving after 10 packets */
	// if (++cnt == 10) {
	// 	LOG_INF("Stopping packet receptions");
	// 	lora_recv_async(dev, NULL, NULL);
	// }
}

int main(void)
{
	int ret;
	if (!gpio_is_ready_dt(&send_button)) {
		LOG_ERR("Error: button device %s is not ready", send_button.port->name);
		return 0;
	}

	ret = gpio_pin_configure_dt(&send_button, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d", ret, send_button.port->name, send_button.pin);
		return 0;
	}

	ret = gpio_pin_interrupt_configure_dt(&send_button, GPIO_INT_EDGE_TO_ACTIVE);
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
	config.tx = false;

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

	LOG_INF("Radio is in receive mode. Press button 1 to send a LoRa packet");

	k_tid_t transmit_tid = k_thread_create(&transmit_thread_id, transmit_stack_area,
											K_THREAD_STACK_SIZEOF(transmit_stack_area),
											transmit_thread, NULL, NULL, NULL,
											PRIORITY, 0, K_FOREVER);

	/* k_thread_name_set expects a k_tid_t (thread id) returned by
	 * k_thread_create. Passing &transmit_thread (a function pointer)
	 * caused an invalid memory access / MPU fault. Use the returned id.
	 */
	k_thread_name_set(transmit_tid, "transmit_thread");

	// k_tid_t receive_tid = k_thread_create(&receive_thread_id, receive_stack_area,
	// 										K_THREAD_STACK_SIZEOF(receive_stack_area),
	// 										receive_thread, NULL, NULL, NULL,
	// 										PRIORITY, 0, K_NO_WAIT);
	// k_thread_name_set(receive_tid, "receive_thread");

	lora_recv_async(lora_dev, lora_receive_cb, NULL);
	k_sleep(K_FOREVER);
	// while (1)
	// {
	// 	k_msleep(SLEEP_TIME_MS);
	// }

	return 0;
}



/*
 * SPDX-License-Identifier: Apache-2.0
 */
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/sys/util.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/lora.h>
#include <zephyr/drivers/gpio.h>
#include <errno.h>
// #include <zephyr/drivers/i2s.h>
// #include <zephyr/audio/codec.h>

#include <inttypes.h>
#include <errno.h>

#include "audio_service.h"
#include "app_common.h"
#include "init_gpio.h"

// #define SW0_NODE	DT_ALIAS(sw0)
// #define SW1_NODE    DT_ALIAS(sw1)

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DEFAULT_RADIO_NODE),
			"No default LoRa radio specified in DT");

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#define MAX_DATA_LEN 128
// #define TRANSMIT	1
// #define RECEIVE 	0

// static struct gpio_callback send_button_cb_data;
// static struct gpio_callback record_button_cb_data;

// static const struct gpio_dt_spec send_button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
// static const struct gpio_dt_spec record_button = GPIO_DT_SPEC_GET(SW1_NODE, gpios);
// static struct gpio_dt_spec send_led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0});

const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);

char data[MAX_DATA_LEN] = {"Chevy LS1 5.7L V8 engine"};
volatile bool config_result = false;

// K_SEM_DEFINE(tx_sem, 0, 1);
K_SEM_DEFINE(record_sem, 0 , 1);

K_THREAD_DEFINE(audio_tid, THREAD_STACK_SIZE, audio_sense_thread, NULL, NULL, NULL, 4, 0, 0);

K_WORK_DEFINE(init_gpio, init_gpio_handler);

LOG_MODULE_REGISTER(main);

void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
		     int16_t rssi, int8_t snr, void *user_data)
{
	// When lora_recv_async is cancelled, may be called with 0 bytes.
	// if (size != 0) {
	// 	printk("RECV %d bytes: ", size);
	// 	for (uint16_t i = 0; i < size; i++)
	// 		printk("0x%02x ",data[i]);
	// 		printk("RSSI = %ddBm, SNR = %ddBm\n", rssi, snr);
	// } 

	printk("\r\nDevice: %s \r\nData: %s\r\nSize: %d\r\nMax bytes: %d\r\nrssi: %d dBm\r\nsnr: %d dB", dev->name, data, strlen(data), size, rssi, snr);
}

int lora_configure(const struct device *dev, bool transmit)
{
	int ret;
	struct lora_modem_config config;

	config.frequency = 865100000;
	config.bandwidth = BW_125_KHZ;
	config.datarate = SF_8;
	config.preamble_len = 8;
	config.coding_rate = CR_4_5;
	config.iq_inverted = false;
	config.public_network = false;
	config.tx_power = 14;
	config.tx = transmit;

	ret = lora_config(dev, &config);
	if (ret < 0) {
		LOG_ERR("LoRa device configuration failed");
		return false;
	}
	return(true);
}

// void send_button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
// {
// 	gpio_pin_toggle_dt(&send_led);
// 	/* Signal transmit. k_sem_give is ISR-safe. */
// 	k_sem_give(&tx_sem);
// }

// void record_button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
// {
// 	LOG_INF("record button pressed");
// 	/* This will start the recording service */
// 	k_sem_give(&record_button);
// }

int main(void)
{
	int ret, bytes;

	// if (!gpio_is_ready_dt(&send_button)) {
	// 	LOG_ERR("Error: send button port is not ready");
	// 	return 0;
	// }

	// ret = gpio_pin_configure_dt(&send_button, GPIO_INPUT);
	// if (ret != 0) {
	// 	LOG_ERR("Error %d: failed to configure %s pin %d", ret, send_button.port->name, send_button.pin);
	// 	return 0;
	// }

	// ret = gpio_pin_interrupt_configure_dt(&send_button, GPIO_INT_EDGE_TO_ACTIVE);
	// if (ret != 0) {
	// 	LOG_ERR("Error %d: failed to configure interrupt on %s pin %d",
	// 		ret, send_button.port->name, send_button.pin);
	// 	return 0;
	// }
	
	// if (!gpio_is_ready_dt(&record_button)) {
	// 	LOG_ERR("Error: button device %s is not ready", record_button.port);
	// 	return 0;
	// }

	// ret = gpio_pin_configure_dt(&record_button, GPIO_INPUT);
	// if (ret != 0) {
	// 	LOG_ERR("Error %d: failed to configure %s pin %d", ret, record_button.port->name, record_button.pin);
	// 	return 0;
	// }

	// ret = gpio_pin_interrupt_configure_dt(&record_button, GPIO_INT_EDGE_TO_ACTIVE);
	// if (ret != 0) {
	// 	LOG_ERR("Error %d: failed to configure interrupt on %s pin %d",
	// 		ret, record_button.port->name, record_button.pin);
	// 	return 0;
	// }

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return 0;
	}

	if (lora_configure(lora_dev, RECEIVE)) {
		LOG_INF("LoRa modem configuring succeeded");
	} else {
		LOG_ERR("Falied configuring LoRa modem");
	}

	// gpio_init_callback(&send_button_cb_data, send_button_pressed, BIT(send_button.pin));
	// gpio_add_callback(send_button.port, &send_button_cb_data);
	// LOG_INF("Set up button at %s pin %d", send_button.port->name, send_button.pin);

	// gpio_init_callback(&record_button_cb_data, record_button_pressed, BIT(record_button.pin));
	// gpio_add_callback(record_button.port, &record_button_cb_data);
	// LOG_INF("Set up button at %s pin %d", record_button.port->name, record_button.pin);

	// if (send_led.port && !gpio_is_ready_dt(&send_led)) {
	// 	LOG_ERR("Error %d: LED device %s is not ready; ignoring it", ret, send_led.port->name);
	// 	send_led.port = NULL;
	// }
	// if (send_led.port) {
	// 	ret = gpio_pin_configure_dt(&send_led, GPIO_OUTPUT);
	// 	if (ret != 0) {
	// 		LOG_ERR("Error %d: failed to configure LED device %s pin %d", ret, send_led.port->name, send_led.pin);
	// 		send_led.port = NULL;
	// 	} else {
	// 		LOG_INF("Set up LED at %s pin %d", send_led.port->name, send_led.pin);
	// 	}
	// }

	k_work_submit(&init_gpio);

	LOG_INF("Radio is in receive mode. Press button 1 to send a LoRa packet");

	/* Start LoRa radion listening */
	ret = lora_recv_async(lora_dev, lora_receive_cb, NULL);
	if (ret < 0) {
		LOG_ERR("LoRa recv_async failed with error code: %d", ret);
	}
	
	while (1)
	{
		/* Wait for the pusbutton to be pressed, once pressed data is transmitted */
		if (k_sem_take(&tx_sem, K_FOREVER) != 0) {
			LOG_ERR("Error taking semaphore");
		}

		/* Cancel reception because transmition has to happen next */
		ret = lora_recv_async(lora_dev, NULL, NULL);
		if (ret < 0) {
			LOG_ERR("LoRa recv_async failed to stop reception with error code: %d", ret);
		}

		/* Reconfigure radio for transmition */
		config_result = lora_configure(lora_dev, TRANSMIT);
		if (!config_result) {
			LOG_ERR("Failed to configure radio for transmission");
		}
		
		/* Transmit data */
		ret = lora_send(lora_dev, data, sizeof(data));
		if (ret < 0){
			LOG_ERR("LoRa send failed: %d", ret);
		}
		else {
			// bytes = sizeof(data);
			bytes = strlen(data);
			LOG_INF("Transmitted bytes = %d", bytes);
			// for (uint16_t i = 0; i < bytes; i++)
			// {
			// 	printk("0x%02x ", data[i]);
			// }
			// printk("\n");
		}

		/* Restart reception */
		lora_configure(lora_dev, RECEIVE);	
		ret = lora_recv_async(lora_dev, lora_receive_cb, NULL);
		if (ret < 0) {
			LOG_ERR("LoRa recv_async failed %d\n", ret);
		} 
	}
	return 0;
}

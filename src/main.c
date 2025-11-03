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

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#define MAX_DATA_LEN 12
#define TRANSMIT	1
#define RECEIVE 	0

#define SW0_NODE	DT_ALIAS(sw0)
	#if !DT_NODE_HAS_STATUS_OKAY(SW0_NODE)
	#error "Unsupported board: sw0 devicetree alias is not defined"
#endif

#define SLEEP_TIME_MS 10
/* size of stack area used by each thread */
#define STACKSIZE 4096
/* scheduling priority used by each thread */
#define PRIORITY 7

static struct gpio_callback send_button_cb_data;

static const struct gpio_dt_spec send_button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_dt_spec send_led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0});

LOG_MODULE_REGISTER(main);

char data[MAX_DATA_LEN] = {"Chevy LS1 5.7L"};

const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);
struct lora_modem_config config;

volatile bool config_result = false;

K_SEM_DEFINE(tx_sem, 0, 1);

void lora_receive_cb(const struct device *dev, uint8_t *data, uint16_t size,
		     int16_t rssi, int8_t snr, void *user_data)
{
	// When lora_recv_async is cancelled, may be called with 0 bytes.
	if (size != 0) {

		printk("RECV %d bytes: ", size);

		for (uint16_t i = 0; i < size; i++)
			printk("0x%02x ",data[i]);
			printk("RSSI = %ddBm, SNR = %ddBm\n", rssi, snr);
	} 

	// LOG_INF("Device %s", dev->name);
	// LOG_INF("Data: %s\r\nSize: %d\r\nrssi: %d dBm\r\nsnr: %d dB", data, size, rssi, snr);
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


// static struct k_thread transmit_thread_id;
// void transmit_thread(void *dummy1, void *dummy2, void *dummy3)
// {
// 	ARG_UNUSED(dummy1);
// 	ARG_UNUSED(dummy2);
// 	ARG_UNUSED(dummy3);
// 	int ret;
// 	/* Thread waits for a semaphore given from the button callback
// 	 * (safer than starting/resuming the thread directly from ISR).
// 	 */
// 	while (1) {
// 		/* Wait until button gives semaphore */
// 		k_sem_take(&tx_sem, K_FOREVER);
// 		/* Stop receiver in order to change config */
// 		ret = lora_recv_async(lora_dev, NULL, NULL);
// 		if (ret < 0) {
// 			LOG_ERR("lora_recv_async(stop) failed: %d", ret);
// 			continue;
// 		}
// 		config.tx = true;
// 		ret = lora_config(lora_dev, &config);
// 		if (ret < 0) {
// 			LOG_ERR("LoRa config (tx) failed: %d", ret);
// 			lora_recv_async(lora_dev, lora_receive_cb, NULL);
// 			continue;
// 		}
// 		LOG_INF("Transmitting");
// 		ret = lora_send(lora_dev, data, MAX_DATA_LEN);
// 		if (ret < 0) {
// 			LOG_ERR("LoRa send failed: %d", ret);
// 		}
// 		/* Switch back to receive mode */
// 		config.tx = false;
// 		ret = lora_config(lora_dev, &config);
// 		if (ret < 0) {
// 			LOG_ERR("LoRa config (rx) failed: %d", ret);
// 		}
// 		/* Enable receiver again after transmission is done */
// 		ret = lora_recv_async(lora_dev, lora_receive_cb, NULL);
// 		if (ret < 0) {
// 			LOG_ERR("lora_recv_async(start) failed: %d", ret);
// 		}
// 	}
// }
// K_THREAD_STACK_DEFINE(transmit_stack_area, STACKSIZE);

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
	/* Signal transmit thread via semaphore. k_sem_give is ISR-safe. */
	k_sem_give(&tx_sem);
}


int main(void)
{
	int ret, bytes;

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

	if (lora_configure(lora_dev, RECEIVE)) {
		LOG_INF("LoRa modem configuring succeeded");
	} else {
		LOG_ERR("Falied configuring LoRa modem");
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

	// k_tid_t transmit_tid = k_thread_create(&transmit_thread_id, transmit_stack_area,
	// 										K_THREAD_STACK_SIZEOF(transmit_stack_area),
	// 										transmit_thread, NULL, NULL, NULL,
	// 										PRIORITY, 0, K_NO_WAIT);
	/* k_thread_name_set expects a k_tid_t (thread id) returned by
	 * k_thread_create. Passing &transmit_thread (a function pointer)
	 * caused an invalid memory access / MPU fault. Use the returned id.
	 */
	// k_thread_name_set(transmit_tid, "transmit_thread");

	// k_tid_t receive_tid = k_thread_create(&receive_thread_id, receive_stack_area,
	// 										K_THREAD_STACK_SIZEOF(receive_stack_area),
	// 										receive_thread, NULL, NULL, NULL,
	// 										PRIORITY, 0, K_NO_WAIT);
	// k_thread_name_set(receive_tid, "receive_thread");

	/* Start LoRa radion listening */
	ret = lora_recv_async(lora_dev, lora_receive_cb, NULL);
	if (ret < 0)
	{
		LOG_ERR("LoRa recv_async failed with error code: %d", ret);
	}
	
	while (1)
	{
		/* Wait for the pusbutton to be pressed, once pressed data is transmitted */
		if (k_sem_take(&tx_sem, K_FOREVER) != 0)
		{
			LOG_ERR("Error taking semaphore");
		}

		/* Cancel reception because transmittion has to happen next */
		ret = lora_recv_async(lora_dev, NULL, NULL);
		if (ret < 0)
		{
			LOG_ERR("LoRa recv_async failed to stop reception with error code: %d", ret);
		}

		/* Reconfigure radio for transmittion */
		config_result = lora_configure(lora_dev, TRANSMIT);
		if (!config_result)
		{
			LOG_ERR("Failed to configure radio for transmittion");
		}
		
		/* Transmit data */
		ret = lora_send(lora_dev, data, sizeof(data));
		if (ret < 0)
		{
			LOG_ERR("LoRa send failed: %d", ret);
		}
		else {
			bytes = sizeof(data);
			LOG_INF("Transmitted bytes = %d", bytes);
			for (uint16_t i = 0; i < bytes; i++)
			{
				printk("0x%02x ", data[i]);
			}
			printk("\n");
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

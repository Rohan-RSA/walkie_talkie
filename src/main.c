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

#define DEFAULT_RADIO_NODE DT_ALIAS(lora0)
BUILD_ASSERT(DT_NODE_HAS_STATUS_OKAY(DEFAULT_RADIO_NODE),
			"No default LoRa radio specified in DT");

#define LOG_LEVEL CONFIG_LOG_DEFAULT_LEVEL
#define MAX_DATA_LEN 128

const struct device *const lora_dev = DEVICE_DT_GET(DEFAULT_RADIO_NODE);

char data[MAX_DATA_LEN] = {"Chevy LS1 5.7L V8 engine"};
volatile bool config_result = false;

K_THREAD_DEFINE(audio_tid, THREAD_STACK_SIZE, audio_sense_thread, NULL, NULL, NULL, 4, 0, 0);

K_WORK_DEFINE(init_gpio, init_gpio_handler);

LOG_MODULE_REGISTER(main, CONFIG_MAIN_LOG_LEVEL);

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

int main(void)
{
	int ret, bytes;
	bool init;

	k_work_submit(&init_gpio);

	if (!device_is_ready(lora_dev)) {
		LOG_ERR("%s Device not ready", lora_dev->name);
		return 0;
	}
	if (lora_configure(lora_dev, RECEIVE)) {
		LOG_DBG("LoRa modem configuring succeeded");
	} else {
		LOG_ERR("Falied configuring LoRa modem");
	}

	init = audio_service_init();
	if (!init) {
		LOG_ERR("Could not initialize I2S device: %d", EIO);
	}
	else LOG_DBG("I2S device initialized");

	

	/* Start LoRa radio listening */
	ret = lora_recv_async(lora_dev, lora_receive_cb, NULL);
	if (ret < 0) {
		LOG_ERR("LoRa recv_async failed with error code: %d", ret);
	}
	LOG_DBG("Radio is in receive mode. Press button 1 to send a LoRa packet");
	
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
			LOG_DBG("Transmitted bytes = %d", bytes);
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

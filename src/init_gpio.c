#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>

#include "init_gpio.h"

#define SW0_NODE	DT_ALIAS(sw0)
#define SW1_NODE    DT_ALIAS(sw1)

K_SEM_DEFINE(tx_sem, 0, 1);
K_SEM_DEFINE(record_sem, 0, 1);

static const struct gpio_dt_spec send_button = GPIO_DT_SPEC_GET_OR(SW0_NODE, gpios, {0});
static struct gpio_dt_spec record_button = GPIO_DT_SPEC_GET(SW1_NODE, gpios);
static struct gpio_dt_spec send_led = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0});

static struct gpio_callback send_button_cb_data;
static struct gpio_callback record_button_cb_data;

LOG_MODULE_REGISTER(init_gpio, CONFIG_INIT_GPIO_LOG_LEVEL);

void record_button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	int val = gpio_pin_get_dt(&record_button);
	if (val < 0) {
		LOG_ERR("Error reading record button: %d", val);
		return;
	}

	if (val == 1) {
		printk("record button pressed (active)\r\n");
		/* This will trigger the recording service */
		k_sem_give(&record_sem);
	} else {
		printk("record button released (inactive)\r\n");
		/* Handle release if needed (stop recording, debounce, etc.) */
	}
}

void send_button_pressed(const struct device *dev, struct gpio_callback *cb, uint32_t pins)
{
	gpio_pin_toggle_dt(&send_led);
	/* Signal transmit. k_sem_give is ISR-safe. */
	k_sem_give(&tx_sem);
}

void init_gpio_handler(struct k_work *work)
{
    int ret;

	if (!gpio_is_ready_dt(&send_button)) {
		LOG_ERR("Error: send button port is not ready");
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
	
	if (!gpio_is_ready_dt(&record_button)) {
		LOG_ERR("Error: button device %s is not ready", record_button.port);
		return 0;
	}

	ret = gpio_pin_configure_dt(&record_button, GPIO_INPUT);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure %s pin %d", ret, record_button.port->name, record_button.pin);
		return 0;
	}

	/* Configure interrupt on both edges and distinguish in the callback by
	 * reading the pin state. Zephyr does not provide per-callback edge
	 * filtering, so use GPIO_INT_EDGE_BOTH and branch on gpio_pin_get_dt().
	 */
	ret = gpio_pin_interrupt_configure_dt(&record_button, GPIO_INT_EDGE_BOTH);
	if (ret != 0) {
		LOG_ERR("Error %d: failed to configure interrupt on %s pin %d",
			ret, record_button.port->name, record_button.pin);
		return 0;
	}

	// if (!device_is_ready(lora_dev)) {
	// 	LOG_ERR("%s Device not ready", lora_dev->name);
	// 	return 0;
	// }

	// if (lora_configure(lora_dev, RECEIVE)) {
	// 	LOG_DBG("LoRa modem configuring succeeded");
	// } else {
	// 	LOG_ERR("Falied configuring LoRa modem");
	// }

	gpio_init_callback(&send_button_cb_data, send_button_pressed, BIT(send_button.pin));
	gpio_add_callback(send_button.port, &send_button_cb_data);
	LOG_DBG("Set up button at %s pin %d", send_button.port->name, send_button.pin);

	gpio_init_callback(&record_button_cb_data, record_button_pressed, BIT(record_button.pin));
	gpio_add_callback(record_button.port, &record_button_cb_data);
	LOG_DBG("Set up button at %s pin %d", record_button.port->name, record_button.pin);

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
			LOG_DBG("Set up LED at %s pin %d", send_led.port->name, send_led.pin);
		}
	}
}
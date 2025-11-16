#ifndef INIT_GPIO_H
#define INIT_GPIO_H

#define TRANSMIT	1
#define RECEIVE 	0

// extern const struct device *const lora_dev;


/* Function prototype */
void init_gpio_handler(struct k_work *work);

// static struct gpio_callback send_button_cb_data;
// static struct gpio_callback record_button_cb_data;

extern struct k_sem tx_sem;
// K_SEM_DEFINE(tx_sem, 0, 1);
// K_SEM_DEFINE(record_sem, 0 , 1);

#endif // INIT_GPIO_H
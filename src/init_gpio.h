#ifndef INIT_GPIO_H
#define INIT_GPIO_H

#define TRANSMIT	1
#define RECEIVE 	0

/* Function prototype */
void init_gpio_handler(struct k_work *work);

extern struct k_sem tx_sem;
extern struct k_sem record_sem;
// K_SEM_DEFINE(tx_sem, 0, 1);
// K_SEM_DEFINE(record_sem, 0 , 1);

#endif // INIT_GPIO_H
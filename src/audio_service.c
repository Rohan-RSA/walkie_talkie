#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/audio/codec.h>
#include <string.h>

#include "audio_service.h"



LOG_MODULE_REGISTER(audio_module);

// static struct gpio_dt_spec record_button = GPIO_DT_SPEC_GET(SW1_NODE, gpios);



#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/audio/codec.h>
#include <string.h>

#include "audio_service.h"

K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

static K_SEM_DEFINE(toggle_transfer, 1, 1);

LOG_MODULE_REGISTER(audio_module);

void audio_service_init()
{
    #if DT_NODE_HAS_STATUS(DT_NODELABEL(i2s0), okay)
        const struct device *const i2s_dev_rx = DEVICE_DT_GET(I2S_RX_NODE);

    #endif
}

void audio_sense_thread(void *arg1, void *arg2, void *arg3)
{

}
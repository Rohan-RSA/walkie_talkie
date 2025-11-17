#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/i2s.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/audio/codec.h>
#include <string.h>

#include "audio_service.h"
#include "init_gpio.h"

K_MEM_SLAB_DEFINE_STATIC(mem_slab, BLOCK_SIZE, BLOCK_COUNT, 4);

static K_SEM_DEFINE(toggle_transfer, 1, 1);



LOG_MODULE_REGISTER(audio_module, CONFIG_AUDIO_SERVICE_LOG_LEVEL);

/* Forward declaration so calls earlier in this file see a matching prototype. */
static bool configure_streams(const struct device *i2s_dev,
                              const struct i2s_config *config);

bool audio_service_init(void)
{
    struct i2s_config config;

    #if DT_NODE_HAS_STATUS(I2S_RX_NODE, okay)
    
        const struct device *const i2s_dev_rx = DEVICE_DT_GET(I2S_RX_NODE);

        if (!device_is_ready(i2s_dev_rx)) {
            LOG_ERR("%s is not ready", i2s_dev_rx->name);
        }
        else LOG_DBG("%s is ready", i2s_dev_rx->name);

        config.word_size = SAMPLE_BIT_WIDTH;
        config.channels = NUMBER_OF_CHANNELS;
        config.format = I2S_FMT_DATA_FORMAT_I2S;
        config.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
        config.frame_clk_freq = SAMPLE_FREQUENCY;
        config.mem_slab = &mem_slab;
        config.block_size = BLOCK_SIZE;
        config.timeout = TIMEOUT;
    
        if(!configure_streams(i2s_dev_rx, &config)){
            return 0;
        }

    #endif

    #if DT_NODE_HAS_STATUS(I2S_TX_NODE, okay)

        const struct device *const i2s_dev_tx = DEVICE_DT_GET(I2S_TX_NODE);

        if (!device_is_ready(i2s_dev_tx)) {
            LOG_ERR("%s is not ready", i2s_dev_tx->name);
        }
        else LOG_DBG("%s is ready", i2s_dev_tx->name);

        config.word_size = SAMPLE_BIT_WIDTH;
        config.channels = NUMBER_OF_CHANNELS;
        config.format = I2S_FMT_DATA_FORMAT_I2S;
        config.options = I2S_OPT_BIT_CLK_MASTER | I2S_OPT_FRAME_CLK_MASTER;
        config.frame_clk_freq = SAMPLE_FREQUENCY;
        config.mem_slab = &mem_slab;
        config.block_size = BLOCK_SIZE;
        config.timeout = TIMEOUT;
    
        if(!configure_streams(i2s_dev_tx, &config)){
            return 0;
        }

    #endif

    return true;
}

static bool configure_streams(const struct device *i2s_dev,
			      /*const struct device *i2s_dev_tx,*/
			      const struct i2s_config *config)
{
	int ret;

    #if DT_NODE_HAS_STATUS(I2S_RX_NODE, okay)

        ret = i2s_configure(i2s_dev, I2S_DIR_RX, config);
        if (ret < 0) {
            LOG_ERR("Failed to configure RX stream: %d", ret);
            return false;
        }

    #endif

    #if DT_NODE_HAS_STATUS(I2S_TX_NODE, okay)
        ret = i2s_configure(i2s_dev, I2S_DIR_TX, config);
        if (ret < 0) {
            LOG_ERR("Failed to configure TX stream: %d", ret);
            return false;
        }
    #endif

    return true;
}
                

void audio_sense_thread(void *arg1, void *arg2, void *arg3)
{

    

}
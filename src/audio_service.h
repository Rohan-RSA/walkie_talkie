#ifndef AUDIO_SERVICE_H
#define AUDIO_SERVICE_H

#include <zephyr/device.h>
#include <zephyr/drivers/i2s.h>

#define I2S_RX_NODE  DT_NODELABEL(i2s_rx)
// #define I2S_TX_NODE  DT_NODELABEL(i2s_tx)

#define SAMPLE_FREQUENCY    44100
#define SAMPLE_BIT_WIDTH    16
#define BYTES_PER_SAMPLE    sizeof(int16_t)
#define NUMBER_OF_CHANNELS  2
/* Such block length provides an echo with the delay of 100 ms. */
#define SAMPLES_PER_BLOCK   ((SAMPLE_FREQUENCY / 10) * NUMBER_OF_CHANNELS)
#define INITIAL_BLOCKS      2
#define TIMEOUT             1000

#define BLOCK_SIZE  (BYTES_PER_SAMPLE * SAMPLES_PER_BLOCK)
#define BLOCK_COUNT (INITIAL_BLOCKS + 4)

#endif // AUDIO_SERVICE_H
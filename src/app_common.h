#ifndef APP_COMMON_H
#define APP_COMMON_H

#define THREAD_STACK_SIZE 1024

#define I2S_RX_NODE  DT_NODELABEL(i2s_rx)
#define I2S_TX_NODE  DT_NODELABEL(i2s_tx)

extern void audio_sense_thread(void *arg1, void *arg2, void *arg3);

#endif //APP_COMMON_H
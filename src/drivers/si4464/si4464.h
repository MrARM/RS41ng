#ifndef __SI4464__H__
#define __SI4464__H__
#include <stdint.h>
#include <stdbool.h>
#include "types.h"
#include <stm32f10x.h>

#define GPIO_SI4063_NSEL GPIOB
#define GPIO_PIN_SI4063_NSEL GPIO_Pin_2

#define GPIO_SI4063_SDN GPIOC
#define GPIO_PIN_SI4063_SDN GPIO_Pin_3


#define RADIO_WRITE_GPIO(state)	palWriteLine(LINE_RADIO_GPIO, state)
#define RADIO_READ_GPIO()		palReadLine(LINE_RADIO_GPIO)
#define inRadioBand(freq) 		(RADIO_MIN_FREQ <= (freq) && (freq) <= RADIO_MAX_FREQ)

#define HIGH	true
#define LOW		false

#define SI4464_STATE_NOCHANGE	0
#define SI4464_STATE_SLEEP		1
#define SI4464_STATE_SPI_ACTIVE	2
#define SI4464_STATE_READY		3
#define SI4464_STATE_READY2		4
#define SI4464_STATE_TX_TUNE	5
#define SI4464_STATE_RX_TUNE	6
#define SI4464_STATE_TX			7
#define SI4464_STATE_RX			8
#define RADIO_MIN_FREQ				144000000
#define RADIO_MAX_FREQ				148000000
#define RADIO_CLK                   8000000

void Si4464_Init(void);
void Si4464_write(uint8_t txData, uint32_t len);
void setFrequency(uint32_t freq, uint16_t shift);
void setShift(uint16_t shift);
void setModemAFSK(void);
void setModemOOK(ook_conf_t* conf);
void setModem2FSK(fsk_conf_t* conf);
void setModem2GFSK(gfsk_conf_t* conf);
void setDeviation(uint32_t deviation);
void setPowerLevel(int8_t level);
void startTx(uint16_t size);
void stopTx(void);
void Si4464_shutdown(void);
bool radioTune(uint32_t frequency, uint16_t shift, int8_t level, uint16_t size);
void Si4464_writeFIFO(uint8_t *msg, uint8_t size);
uint8_t Si4464_freeFIFO(void);
uint8_t Si4464_getState(void);
int16_t Si4464_getTemperature(void);
int16_t Si4464_getLastTemperature(void);

#endif


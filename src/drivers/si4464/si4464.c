/**
 * Si4464 driver specialized for APRS transmissions. Modulation concept has been taken
 * from Stefan Biereigel DK3SB.
 * @see http://www.github.com/thasti/utrak
 */

#include "si4464.h"
#include <stm32f10x_gpio.h>
#include "hal/spi.h"
#include "hal/delay.h"
#include <string.h>



static uint32_t outdiv;
static bool initialized = false;
static int16_t lastTemp;

/**
 * Initializes Si4464 transceiver chip. Adjustes the frequency which is shifted by variable
 * oscillator voltage.
 * @param mv Oscillator voltage in mv
 */
void Si4464_Init(void) {
    GPIO_InitTypeDef gpio_init;

    // Si chip select pin
    gpio_init.GPIO_Pin = GPIO_PIN_SI4063_NSEL;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIO_SI4063_NSEL, &gpio_init);

    // Si shutdown pin
    gpio_init.GPIO_Pin = GPIO_PIN_SI4063_SDN;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIO_SI4063_SDN, &gpio_init);


    //palSetLine(LINE_RADIO_CS);

    // Reset radio
    GPIO_SetBits(GPIO_SI4063_SDN, GPIO_PIN_SI4063_SDN);
    //palSetLine(LINE_OSC_EN); // Activate Oscillator // Oscillator tied to chip, no soft control.
    delay_ms(10);

    // Power up transmitter
    GPIO_ResetBits(GPIO_SI4063_SDN, GPIO_PIN_SI4063_SDN);	// Radio SDN low (power up transmitter)
    delay_ms(10);		// Wait for transmitter to power up

    // Power up (transmits oscillator type)
    uint8_t x3 = (RADIO_CLK >> 24) & 0x0FF;
    uint8_t x2 = (RADIO_CLK >> 16) & 0x0FF;
    uint8_t x1 = (RADIO_CLK >>  8) & 0x0FF;
    uint8_t x0 = (RADIO_CLK >>  0) & 0x0FF;
    uint8_t init_command[] = {0x02, 0x01, 0x01, x3, x2, x1, x0};
    Si4464_write(init_command, 7);
    delay_ms(25);

    // Set transmitter GPIOs
    uint8_t gpio_pin_cfg_command[] = {
            0x13,	// Command type = GPIO settings
            0x00,	// GPIO0        0 - PULL_CTL[1bit] - GPIO_MODE[6bit]
            0x23,	// GPIO1        0 - PULL_CTL[1bit] - GPIO_MODE[6bit]
            0x00,	// GPIO2        0 - PULL_CTL[1bit] - GPIO_MODE[6bit]
            0x00,	// GPIO3        0 - PULL_CTL[1bit] - GPIO_MODE[6bit]
            0x00,	// NIRQ
            0x00,	// SDO
            0x00	// GEN_CONFIG
    };
    Si4464_write(gpio_pin_cfg_command, 8);
    delay_ms(25);

    // Set FIFO empty interrupt threshold (32 byte)
    uint8_t set_fifo_irq[] = {0x11, 0x12, 0x01, 0x0B, 0x20};
    Si4464_write(set_fifo_irq, 5);

    // Set FIFO to 129 byte
    uint8_t set_129byte[] = {0x11, 0x00, 0x01, 0x03, 0x10};
    Si4464_write(set_129byte, 5);

    // Reset FIFO
    uint8_t reset_fifo[] = {0x15, 0x01};
    Si4464_write(reset_fifo, 2);
    uint8_t unreset_fifo[] = {0x15, 0x00};
    Si4464_write(unreset_fifo, 2);

    // Disable preamble
    uint8_t disable_preamble[] = {0x11, 0x10, 0x01, 0x00, 0x00};
    Si4464_write(disable_preamble, 5);

    // Do not transmit sync word
    uint8_t no_sync_word[] = {0x11, 0x11, 0x01, 0x00, (0x01 << 7)};
    Si4464_write(no_sync_word, 5);

    // Setup the NCO modulo and oversampling mode
    uint32_t s = RADIO_CLK / 10;
    uint8_t f3 = (s >> 24) & 0xFF;
    uint8_t f2 = (s >> 16) & 0xFF;
    uint8_t f1 = (s >>  8) & 0xFF;
    uint8_t f0 = (s >>  0) & 0xFF;
    uint8_t setup_oversampling[] = {0x11, 0x20, 0x04, 0x06, f3, f2, f1, f0};
    Si4464_write(setup_oversampling, 8);

    // transmit LSB first
    uint8_t use_lsb_first[] = {0x11, 0x12, 0x01, 0x06, 0x01};
    Si4464_write(use_lsb_first, 5);

    // Temperature readout
    lastTemp = Si4464_getTemperature();
    //TRACE_INFO("SI   > Transmitter temperature %d degC", lastTemp/100);
    initialized = true;
}

void Si4464_write(uint8_t txData, uint32_t len) {
//    // Transmit data by SPI
//    uint8_t rxData[len];
//
//    // SPI transfer
//    spiAcquireBus(&SPID3);
//    spiStart(&SPID3, getSPIDriver());
//
//    spiSelect(&SPID3);
//    spiExchange(&SPID3, len, txData, rxData);
//    spiUnselect(&SPID3);
//
//    // Reqest ACK by Si4464
//    rxData[1] = 0x00;
//    while(rxData[1] != 0xFF) {
//
//        // Request ACK by Si4464
//        uint8_t rx_ready[] = {0x44};
//
//        // SPI transfer
//        spiSelect(&SPID3);
//        spiExchange(&SPID3, 3, rx_ready, rxData);
//        spiUnselect(&SPID3);
//    }
//    spiStop(&SPID3);
//    spiReleaseBus(&SPID3);
    spi_send_and_receive(GPIO_SI4063_NSEL, GPIO_PIN_SI4063_NSEL, (uint16_t) txData);
}

/**
 * Read register from Si4464. First Register CTS is included.
 */
void Si4464_read(uint8_t* txData, uint32_t txlen, uint8_t* rxData, uint32_t rxlen) {
//    // Transmit data by SPI
//    uint8_t null_spi[txlen];
//    // SPI transfer
//    spiAcquireBus(&SPID3);
//    spiStart(&SPID3, getSPIDriver());
//
//    spiSelect(&SPID3);
//    spiExchange(&SPID3, txlen, txData, null_spi);
//    spiUnselect(&SPID3);
//
//    // Reqest ACK by Si4464
//    rxData[1] = 0x00;
//    while(rxData[1] != 0xFF) {
//
//        // Request ACK by Si4464
//        uint16_t rx_ready[rxlen];
//        rx_ready[0] = 0x44;
//
//        // SPI transfer
//        spiSelect(&SPID3);
//        spiExchange(&SPID3, rxlen, rx_ready, rxData);
//        spiUnselect(&SPID3);
//    }
//    spiStop(&SPID3);
//    spiReleaseBus(&SPID3);

}

void setFrequency(uint32_t freq, uint16_t shift) {
    // Set the output divider according to recommended ranges given in Si4464 datasheet
    uint32_t band = 0;
    if(freq < 705000000UL) {outdiv = 6;  band = 1;};
    if(freq < 525000000UL) {outdiv = 8;  band = 2;};
    if(freq < 353000000UL) {outdiv = 12; band = 3;};
    if(freq < 239000000UL) {outdiv = 16; band = 4;};
    if(freq < 177000000UL) {outdiv = 24; band = 5;};

    // Set the band parameter
    uint32_t sy_sel = 8;
    uint8_t set_band_property_command[] = {0x11, 0x20, 0x01, 0x51, (band + sy_sel)};
    Si4464_write(set_band_property_command, 5);

    // Set the PLL parameters
    uint32_t f_pfd = 2 * RADIO_CLK / outdiv;
    uint32_t n = ((uint32_t)(freq / f_pfd)) - 1;
    float ratio = (float)freq / (float)f_pfd;
    float rest  = ratio - (float)n;

    uint32_t m = (uint32_t)(rest * 524288UL);
    uint32_t m2 = m >> 16;
    uint32_t m1 = (m - m2 * 0x10000) >> 8;
    uint32_t m0 = (m - m2 * 0x10000 - (m1 << 8));

    uint32_t channel_increment = 524288 * outdiv * shift / (2 * RADIO_CLK);
    uint8_t c1 = channel_increment / 0x100;
    uint8_t c0 = channel_increment - (0x100 * c1);

    uint8_t set_frequency_property_command[] = {0x11, 0x40, 0x04, 0x00, n, m2, m1, m0, c1, c0};
    Si4464_write(set_frequency_property_command, 10);

    uint32_t x = ((((uint32_t)1 << 19) * outdiv * 1300.0)/(2*RADIO_CLK))*2;
    uint8_t x2 = (x >> 16) & 0xFF;
    uint8_t x1 = (x >>  8) & 0xFF;
    uint8_t x0 = (x >>  0) & 0xFF;
    uint8_t set_deviation[] = {0x11, 0x20, 0x03, 0x0a, x2, x1, x0};
    Si4464_write(set_deviation, 7);
}

void setShift(uint16_t shift) {
    if(!shift)
        return;

    float units_per_hz = (( 0x40000 * outdiv ) / (float)RADIO_CLK);

    // Set deviation for 2FSK
    uint32_t modem_freq_dev = (uint32_t)(units_per_hz * shift / 2.0 );
    uint8_t modem_freq_dev_0 = 0xFF & modem_freq_dev;
    uint8_t modem_freq_dev_1 = 0xFF & (modem_freq_dev >> 8);
    uint8_t modem_freq_dev_2 = 0xFF & (modem_freq_dev >> 16);

    uint8_t set_modem_freq_dev_command[] = {0x11, 0x20, 0x03, 0x0A, modem_freq_dev_2, modem_freq_dev_1, modem_freq_dev_0};
    Si4464_write(set_modem_freq_dev_command, 7);
}

void setModemAFSK(void) {
    // Setup the NCO data rate for APRS
    uint8_t setup_data_rate[] = {0x11, 0x20, 0x03, 0x03, 0x00, 0x33, 0x90};
    Si4464_write(setup_data_rate, 7);

    // Use 2GFSK from FIFO (PH)
    uint8_t use_2gfsk[] = {0x11, 0x20, 0x01, 0x00, 0x03};
    Si4464_write(use_2gfsk, 5);

    // Set AFSK filter
    uint8_t coeff[] = {0x81, 0x9f, 0xc4, 0xee, 0x18, 0x3e, 0x5c, 0x70, 0x76};
    uint8_t i;
    for(i=0; i<sizeof(coeff); i++) {
        uint8_t msg[] = {0x11, 0x20, 0x01, 0x17-i, coeff[i]};
        Si4464_write(msg, 5);
    }
}

void setModemOOK(ook_conf_t* conf) {
    // Setup the NCO data rate for 2FSK
    uint16_t speed = conf->speed * 5 / 6;
    uint8_t setup_data_rate[] = {0x11, 0x20, 0x03, 0x03, 0x00, 0x00, (uint8_t)speed};
    Si4464_write(setup_data_rate, 7);

    // Use 2FSK from FIFO (PH)
    uint8_t use_ook[] = {0x11, 0x20, 0x01, 0x00, 0x01};
    Si4464_write(use_ook, 5);
}

void setModem2FSK(fsk_conf_t* conf) {
    // Setup the NCO data rate for 2FSK
    uint8_t setup_data_rate[] = {0x11, 0x20, 0x03, 0x03, (uint8_t)(conf->baud >> 16), (uint8_t)(conf->baud >> 8), (uint8_t)conf->baud};
    Si4464_write(setup_data_rate, 7);

    // Use 2FSK from FIFO (PH)
    uint8_t use_2fsk[] = {0x11, 0x20, 0x01, 0x00, 0x02};
    Si4464_write(use_2fsk, 5);
}

void setModem2GFSK(gfsk_conf_t* conf) {
    // Setup the NCO data rate for 2GFSK
    uint8_t setup_data_rate[] = {0x11, 0x20, 0x03, 0x03, (uint8_t)(conf->speed >> 16), (uint8_t)(conf->speed >> 8), (uint8_t)conf->speed};
    Si4464_write(setup_data_rate, 7);

    // Use 2GFSK from FIFO (PH)
    uint8_t use_2gfsk[] = {0x11, 0x20, 0x01, 0x00, 0x02};
    Si4464_write(use_2gfsk, 5);
}

void setPowerLevel(int8_t level) {
    // Set the Power
    uint8_t set_pa_pwr_lvl_property_command[] = {0x11, 0x22, 0x01, 0x01, level};
    Si4464_write(set_pa_pwr_lvl_property_command, 5);
}

void startTx(uint16_t size) {
    //palClearLine(LINE_IO_LED1);	// Set indication LED

    uint8_t change_state_command[] = {0x31, 0x00, 0x30, (size >> 8) & 0x1F, size & 0xFF};
    Si4464_write(change_state_command, 5);
}

void stopTx(void) {
    //palSetLine(LINE_IO_LED1);	// Set indication LED

    uint8_t change_state_command[] = {0x34, 0x03};
    Si4464_write(change_state_command, 2);
}

void Si4464_shutdown(void) {
    initialized = false;
}

/**
 * Tunes the radio and activates transmission.
 * @param frequency Transmission frequency in Hz
 * @param shift Shift of FSK in Hz
 * @param level Transmission power level in dBm
 */
bool radioTune(uint32_t frequency, uint16_t shift, int8_t level, uint16_t size) {
    // Tracing
    //TRACE_INFO("SI   > Tune Si4464");

    if(!inRadioBand(frequency)) {
      //  TRACE_ERROR("SI   > Frequency out of range");
        //TRACE_ERROR("SI   > abort transmission");
        //return false;
    }

    setFrequency(frequency, shift);	// Set frequency
    setShift(shift);				// Set shift
    setPowerLevel(level);			// Set power level

    startTx(size);
    return true;
}

void Si4464_writeFIFO(uint8_t *msg, uint8_t size) {
    uint8_t write_fifo[size+1];
    write_fifo[0] = 0x66;
    memcpy(&write_fifo[1], msg, size);
    Si4464_write(write_fifo, size+1);
}

/**
  * Returns free space in FIFO of Si4464
  */
uint8_t Si4464_freeFIFO(void) {
    uint8_t fifo_info[2] = {0x15, 0x00};
    uint8_t rxData = spi_send_and_receive(GPIO_SI4063_NSEL, GPIO_PIN_SI4063_NSEL, fifo_info);
    return rxData << 3;
}

/**
  * Returns internal state of Si4464
  */
uint8_t Si4464_getState(void) {
    uint8_t fifo_info[1] = {0x33};
    uint8_t rxData = spi_send_and_receive(GPIO_SI4063_NSEL, GPIO_PIN_SI4063_NSEL, 0x33);
    return rxData << 2;
}

int16_t Si4464_getTemperature(void) {
    uint8_t txData[2] = {0x14, 0x10};
    uint8_t rxData = spi_send_and_receive(GPIO_SI4063_NSEL, GPIO_PIN_SI4063_NSEL, txData);
    //Si4464_read(txData, 2, rxData, 8);
    uint16_t adc = rxData | ((rxData & 0x7) << 8);
    return (89900*adc)/4096 - 29300;
}

int16_t Si4464_getLastTemperature(void) {
    return lastTemp;
}

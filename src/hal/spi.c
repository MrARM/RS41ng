#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_spi.h>

#include "spi.h"

void spi_init()
{
    GPIO_InitTypeDef gpio_init;

    // SPI1_SCK & SPI1_MOSI
    gpio_init.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init);

    // SPI1_MISO
    gpio_init.GPIO_Pin = GPIO_Pin_6;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, ENABLE);

    SPI_InitTypeDef spi_init;
    SPI_StructInit(&spi_init);

    spi_init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    spi_init.SPI_Mode = SPI_Mode_Master;
    spi_init.SPI_DataSize = SPI_DataSize_16b;
    spi_init.SPI_CPOL = SPI_CPOL_Low;
    spi_init.SPI_CPHA = SPI_CPHA_1Edge;
    spi_init.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_256;
    spi_init.SPI_FirstBit = SPI_FirstBit_MSB;
    spi_init.SPI_CRCPolynomial = 7;
    SPI_Init(SPI1, &spi_init);

    SPI_SSOutputCmd(SPI1, ENABLE);

    SPI_Cmd(SPI1, ENABLE);
    SPI_Init(SPI1, &spi_init);
}

void spi_uninit()
{
    SPI_I2S_DeInit(SPI1);
    SPI_Cmd(SPI1, DISABLE);
    SPI_SSOutputCmd(SPI1, DISABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1, DISABLE);

    GPIO_InitTypeDef gpio_init;

    gpio_init.GPIO_Pin = GPIO_Pin_6;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init);

    gpio_init.GPIO_Pin = GPIO_Pin_7;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP; // was: GPIO_Mode_Out_PP; // GPIO_Mode_AF_PP
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init);
}

inline void spi_send(uint16_t data)
{
    // Wait for TX buffer
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, data);
}

inline uint8_t spi_receive()
{
    // Wait for data in RX buffer
    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    return (uint8_t) SPI_I2S_ReceiveData(SPI1);
}

uint8_t spi_send_and_receive(GPIO_TypeDef *gpio_cs, uint16_t pin_cs, uint16_t data) {
    GPIO_ResetBits(gpio_cs, pin_cs);

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
    SPI_I2S_SendData(SPI1, data);

    while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
    GPIO_SetBits(gpio_cs, pin_cs);
    return (uint8_t) SPI_I2S_ReceiveData(SPI1);
}

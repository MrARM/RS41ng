#include <stm32f10x_rcc.h>
#include <stm32f10x_gpio.h>
#include <stm32f10x_usart.h>

#include "usart_ext.h"

// USART1 is the external serial port, which shares TX/RX pins with the external I²C bus.

void usart_ext_init(uint32_t baud_rate)
{
    GPIO_InitTypeDef gpio_init;

    // USART1 TX
    gpio_init.GPIO_Pin = GPIO_Pin_9;
    gpio_init.GPIO_Mode = GPIO_Mode_AF_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio_init);

    // USART1 RX
    gpio_init.GPIO_Pin = GPIO_Pin_10;
    gpio_init.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio_init);

    NVIC_DisableIRQ(USART1_IRQn);
    USART_Cmd(USART1, DISABLE);

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, ENABLE);

    USART_InitTypeDef usart_init;
    usart_init.USART_BaudRate = baud_rate;
    usart_init.USART_WordLength = USART_WordLength_8b;
    usart_init.USART_StopBits = USART_StopBits_1;
    usart_init.USART_Parity = USART_Parity_No;
    usart_init.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart_init.USART_Mode = USART_Mode_Tx; // Enable only transmit for now | USART_Mode_Rx;
    USART_Init(USART1, &usart_init);

    USART_Cmd(USART1, ENABLE);
}

void usart_ext_uninit()
{
    USART_Cmd(USART1, DISABLE);
    USART_DeInit(USART1);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1, DISABLE);
}

void usart_ext_enable(bool enabled)
{
    USART_Cmd(USART1, enabled ? ENABLE : DISABLE);
}

void usart_ext_send_byte(uint8_t data)
{
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}
    USART_SendData(USART1, data);
    while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}
}

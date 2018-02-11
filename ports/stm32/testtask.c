/*
 * testtask.c
 *
 *  Created on: 11.02.2018
 *      Author: jesstr
 */
#include "FreeRTOS.h"
#include "task.h"

#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"

void TASK_Led(void *pvParameters) {
	GPIO_InitTypeDef GPIO_InitStruct;

	/*Configure GPIO pins : PD15 */
	GPIO_InitStruct.Pin = GPIO_PIN_15;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
	HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

	for (;;) {
		HAL_GPIO_TogglePin(GPIOD, GPIO_PIN_15);
		vTaskDelay(200);
	}
}

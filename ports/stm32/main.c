/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2013, 2014 Damien P. George
 * Copyright (c) 2015 Daniel Campora
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <stdint.h>
#include <string.h>
#include <ctype.h>

#include "FreeRTOS.h"
#include "task.h"

#include "py/mpconfig.h"
#include "py/mphal.h"
#include "mptask.h"
#include "systick.h"
#include "pendsv.h"
#include "pybthread.h"
#include "gccollect.h"
#include "modmachine.h"
#include "i2c.h"
#include "spi.h"
#include "uart.h"
#include "timer.h"
#include "led.h"
#include "pin.h"
#include "extint.h"
#include "usrsw.h"
#include "usb.h"
#include "rtc.h"
#include "storage.h"
#include "sdcard.h"
#include "rng.h"
#include "accel.h"
#include "servo.h"
#include "dac.h"
#include "can.h"
#include "modnetwork.h"
#include "testtask.h"

void SystemClock_Config(void);

// This is the static memory (TCB and stack) for the idle task
static StaticTask_t xIdleTaskTCB __attribute__ ((section (".rtos_heap")));
static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE] __attribute__ ((section (".rtos_heap"))) __attribute__((aligned (8)));

#ifdef DEBUG
OsiTaskHandle   mpTaskHandle;
#endif

// This is the FreeRTOS heap, defined here so we can put it in a special segment
uint8_t ucHeap[ configTOTAL_HEAP_SIZE ] __attribute__ ((section (".rtos_heap"))) __attribute__((aligned (8)));

// This is the static memory (TCB and stack) for the main MicroPython task
StaticTask_t mpTaskTCB __attribute__ ((section (".rtos_heap")));
StackType_t mpTaskStack[MICROPY_TASK_STACK_LEN] __attribute__ ((section (".rtos_heap"))) __attribute__((aligned (8)));

StaticTask_t LedTaskTCB __attribute__ ((section (".rtos_heap")));
StackType_t LedTaskStack[configMINIMAL_STACK_SIZE] __attribute__ ((section (".rtos_heap"))) __attribute__((aligned (8)));


//__attribute__ ((section (".boot")))
int main (void) {
	// TODO disable JTAG
	__disable_irq();
	HAL_NVIC_SetPriorityGrouping(0);
	__enable_irq();

	HAL_Init();

	// set the system clock to be HSE
	SystemClock_Config();

	// enable GPIO clocks
	__GPIOA_CLK_ENABLE();
	__GPIOB_CLK_ENABLE();
	__GPIOC_CLK_ENABLE();
	__GPIOD_CLK_ENABLE();

#if defined(MCU_SERIES_F4) ||  defined(MCU_SERIES_F7)
    #if defined(__HAL_RCC_DTCMRAMEN_CLK_ENABLE)
    // The STM32F746 doesn't really have CCM memory, but it does have DTCM,
    // which behaves more or less like normal SRAM.
    __HAL_RCC_DTCMRAMEN_CLK_ENABLE();
    #elif defined(CCMDATARAM_BASE)
    // enable the CCM RAM
    __HAL_RCC_CCMDATARAMEN_CLK_ENABLE();
    #endif
#endif

#if defined(MICROPY_BOARD_EARLY_INIT)
MICROPY_BOARD_EARLY_INIT();
#endif

	// basic sub-system init
#if MICROPY_PY_THREAD
	pyb_thread_init(&pyb_thread_main);
#endif
	pendsv_init();
	led_init();
#if MICROPY_HW_HAS_SWITCH
	switch_init0();
#endif
	machine_init();
#if MICROPY_HW_ENABLE_RTC
	rtc_init_start(false);
#endif
	spi_init0();
#if MICROPY_HW_ENABLE_HW_I2C
	i2c_init0();
#endif
#if MICROPY_HW_HAS_SDCARD
	sdcard_init();
#endif
	storage_init();

#ifndef DEBUG
    void* mpTaskHandle;
#endif
    mpTaskHandle = xTaskCreateStatic(TASK_MicroPython, "MicroPy",
        MICROPY_TASK_STACK_LEN, NULL, MICROPY_TASK_PRIORITY, mpTaskStack, &mpTaskTCB);
    (void)mpTaskHandle;

    void* LedTaskHandle = xTaskCreateStatic(TASK_Led, "LedTask",
		configMINIMAL_STACK_SIZE, NULL, 2, LedTaskStack, &LedTaskTCB);
    (void)LedTaskHandle;

    vTaskStartScheduler();

    for (;;);
}

// We need this when configSUPPORT_STATIC_ALLOCATION is enabled
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t **ppxIdleTaskStackBuffer,
                                    uint32_t *pulIdleTaskStackSize ) {
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

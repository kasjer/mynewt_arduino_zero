/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <assert.h>
#include <stddef.h>
#include <inttypes.h>
#include "syscfg/syscfg.h"
#include "hal/hal_bsp.h"
#include "bsp/bsp.h"
#include <os/os_cputime.h>
#include "sysinit/sysinit.h"
#include "hal/hal_flash.h"
#include "mcu/samd21.h"
#include "bsp/bsp.h"

/*
 * hw/mcu/atmel/samd21xx/src/sam0/drivers/tc/tc.h
 */
#include <tc.h>

#include "mcu/samd21_hal.h"
#include "hal/hal_spi.h"
#include "mcu/hal_spi.h"

/*
 * hw/mcu/atmel/samd21xx/src/sam0/drivers/sercom/usart/usart.h
 */
#include <usart.h>
#include <mcu/hal_uart.h>

#include <os/os_dev.h>

#if MYNEWT_VAL(UART_0)
#include <uart/uart.h>
#include <uart_hal/uart_hal.h>
#endif

#if MYNEWT_VAL(UART_0)
#if !MYNEWT_VAL(BOOT_LOADER) || MYNEWT_VAL(BOOT_SERIAL)
static struct uart_dev hal_uart0;
#endif
#endif

#if MYNEWT_VAL(SPI_0)
static struct samd21_spi_config ext_spi_cfg = {
    .dipo = 3,
    .dopo = 0,
    .pad0_pinmux = PINMUX_PA04D_SERCOM0_PAD0,   /* DO */
    .pad1_pinmux = PINMUX_PA05D_SERCOM0_PAD1,   /* SCK */
#if MYNEWT_VAL(SPI_0_TYPE) == HAL_SPI_TYPE_MASTER
    .pad2_pinmux = PINMUX_UNUSED,               /* SS */
#else
    .pad2_pinmux = PINMUX_PA06D_SERCOM0_PAD2,   /* SS */
#endif
    .pad3_pinmux = PINMUX_PA07D_SERCOM0_PAD3    /* DI */
};
#endif

#if MYNEWT_VAL(SPI_1)
static struct samd21_spi_config ext_spi_cfg = {
    .dipo = 3,
    .dopo = 0,
    .pad0_pinmux = PINMUX_PA00D_SERCOM1_PAD0,   /* DO */
    .pad1_pinmux = PINMUX_PA01D_SERCOM1_PAD1,   /* SCK */
#if MYNEWT_VAL(SPI_1_TYPE) == HAL_SPI_TYPE_MASTER
    .pad2_pinmux = PINMUX_UNUSED,               /* SS */
#else
    .pad2_pinmux = PINMUX_PA02D_SERCOM1_PAD2,   /* SS */
#endif
    .pad3_pinmux = PINMUX_PA31D_SERCOM1_PAD3    /* DI */
};
#endif

#if MYNEWT_VAL(SPI_2)
static struct samd21_spi_config winc1500_spi_cfg = {
    .dipo = 3,
    .dopo = 0,
    .pad0_pinmux = PINMUX_PA12C_SERCOM2_PAD0,   /* DO */
    .pad1_pinmux = PINMUX_PA13C_SERCOM2_PAD1,   /* SCK */
#if MYNEWT_VAL(SPI_2_TYPE) == HAL_SPI_TYPE_MASTER
    .pad2_pinmux = PINMUX_UNUSED,               /* SS */
#else
    .pad2_pinmux = PINMUX_PA14C_SERCOM2_PAD2,   /* SS */
#endif
    .pad3_pinmux = PINMUX_PA15C_SERCOM2_PAD3    /* DI */

};
#endif

#if MYNEWT_VAL(SPI_3)
static struct samd21_spi_config ext_spi_cfg = {
    .dipo = 3,
    .dopo = 0,
    .pad0_pinmux = PINMUX_PA16D_SERCOM3_PAD0,   /* MOSI */
    .pad1_pinmux = PINMUX_PA17D_SERCOM3_PAD1,   /* SCK */
#if MYNEWT_VAL(SPI_3_TYPE) == HAL_SPI_TYPE_MASTER
    .pad2_pinmux = PINMUX_UNUSED,               /* SS */
#else
    .pad2_pinmux = PINMUX_PA18D_SERCOM3_PAD2,   /* SS */
#endif
    .pad3_pinmux = PINMUX_PA19D_SERCOM3_PAD3    /* MISO */
};
#endif

#if MYNEWT_VAL(UART_0)
#if !MYNEWT_VAL(BOOT_LOADER) || MYNEWT_VAL(BOOT_SERIAL)
static const struct samd21_uart_config uart_cfgs[] = {
    [0] = {
        .suc_sercom = SERCOM5,
        .suc_mux_setting = USART_RX_3_TX_2_XCK_3,
        .suc_generator_source = GCLK_GENERATOR_0,
        .suc_sample_rate = USART_SAMPLE_RATE_16X_ARITHMETIC,
        .suc_sample_adjustment = USART_SAMPLE_ADJUSTMENT_7_8_9,
        .suc_pad0 = 0,
        .suc_pad1 = 0,
        .suc_pad2 = PINMUX_PB22D_SERCOM5_PAD2,
        .suc_pad3 = PINMUX_PB23D_SERCOM5_PAD3
    }
};
#endif
#endif

/*
 * What memory to include in coredump.
 */
static const struct hal_bsp_mem_dump dump_cfg[] = {
    [0] = {
	.hbmd_start = &_ram_start,
        .hbmd_size = RAM_SIZE
    }
};

const struct hal_flash *
hal_bsp_flash_dev(uint8_t id)
{
    /*
     * Internal flash mapped to id 0.
     */
    if (id != 0) {
        return NULL;
    }
    return &samd21_flash_dev;
}

const struct hal_bsp_mem_dump *
hal_bsp_core_dump(int *area_cnt)
{
    *area_cnt = sizeof(dump_cfg) / sizeof(dump_cfg[0]);
    return dump_cfg;
}

/**
 * Returns the configured priority for the given interrupt. If no priority
 * configured, return the priority passed in
 *
 * @param irq_num
 * @param pri
 *
 * @return uint32_t
 */
uint32_t
hal_bsp_get_nvic_priority(int irq_num, uint32_t pri)
{
    /* Add any interrupt priorities configured by the bsp here */
    return pri;
}

void
hal_bsp_init(void)
{
#if !MYNEWT_VAL(BOOT_LOADER) || MYNEWT_VAL(BOOT_SERIAL)
    int rc;
#if MYNEWT_VAL(TIMER_0) || MYNEWT_VAL(TIMER_1) || MYNEWT_VAL(TIMER_2)
    struct samd21_timer_cfg tmr_cfg;
#endif
    (void)rc;

#if MYNEWT_VAL(UART_0)
    rc = os_dev_create((struct os_dev *) &hal_uart0, "uart0",
      OS_DEV_INIT_PRIMARY, 0, uart_hal_init, (void *)&uart_cfgs[0]);
    SYSINIT_PANIC_ASSERT(rc == 0);
#endif
#if MYNEWT_VAL(TIMER_0)
    tmr_cfg.clkgen = GCLK_GENERATOR_2;
    tmr_cfg.src_clock = GCLK_SOURCE_OSC8M;
    tmr_cfg.hwtimer = TC3;
    tmr_cfg.irq_num = TC3_IRQn;
    rc = hal_timer_init(0, &tmr_cfg);
    assert(rc == 0);
#endif
#if MYNEWT_VAL(TIMER_1)
    tmr_cfg.clkgen = GCLK_GENERATOR_5;
    tmr_cfg.src_clock = GCLK_SOURCE_OSC8M;
    tmr_cfg.hwtimer = TC4;
    tmr_cfg.irq_num = TC4_IRQn;
    rc = hal_timer_init(1, &tmr_cfg);
    assert(rc == 0);
#endif
#if MYNEWT_VAL(TIMER_2)
    tmr_cfg.clkgen = GCLK_GENERATOR_6;
    tmr_cfg.src_clock = GCLK_SOURCE_OSC8M;
    tmr_cfg.hwtimer = TC5;
    tmr_cfg.irq_num = TC5_IRQn;
    rc = hal_timer_init(2, &tmr_cfg);
    assert(rc == 0);
#endif

#if (MYNEWT_VAL(OS_CPUTIME_TIMER_NUM) >= 0)
    rc = os_cputime_init(MYNEWT_VAL(OS_CPUTIME_FREQ));
    assert(rc == 0);
#endif
#endif

#if !MYNEWT_VAL(BOOT_LOADER)
#if MYNEWT_VAL(SPI_0)
    rc = hal_spi_init(0, &ext_spi_cfg, MYNEWT_VAL(SPI_0_TYPE));
    SYSINIT_PANIC_ASSERT(rc == 0);
#endif

#if MYNEWT_VAL(SPI_1)
    rc = hal_spi_init(1, &ext_spi_cfg, MYNEWT_VAL(SPI_1_TYPE));
    SYSINIT_PANIC_ASSERT(rc == 0);
#endif

#if MYNEWT_VAL(SPI_2)
    rc = hal_spi_init(2, &winc1500_spi_cfg, MYNEWT_VAL(SPI_2_TYPE));
    SYSINIT_PANIC_ASSERT(rc == 0);
#endif

#if MYNEWT_VAL(SPI_3)
    rc = hal_spi_init(3, &ext_spi_cfg, MYNEWT_VAL(SPI_3_TYPE));
    SYSINIT_PANIC_ASSERT(rc == 0);
#endif
#endif
}

void
hal_bsp_deinit(void)
{

}

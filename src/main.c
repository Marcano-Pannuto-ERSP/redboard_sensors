// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText 2023 Kristin Ebuengan
// SPDX-FileCopyrightText 2023 Melody Gill
// SPDX-FileCopyrightText 2023 Gabriel Marcano

/*
 * This is an edited file of main.c from https://github.com/gemarcano/redboard_lora_example
 * which this repo was forked from
 *
 * Tests getting temperature and pressure from the BMP280 sensor.
 */

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>

#include <spi.h>
#include <uart.h>
#include <am1815.h>
#include <bmp280.h>

static struct uart uart;
static struct spi_bus spi;
static struct spi_device spi_sensor;
static struct bmp280 sensor;

int main(void)
{
	// Prepare MCU by init-ing clock, cache, and power level operation
	am_hal_clkgen_control(AM_HAL_CLKGEN_CONTROL_SYSCLK_MAX, 0);
	am_hal_cachectrl_config(&am_hal_cachectrl_defaults);
	am_hal_cachectrl_enable();
	am_bsp_low_power_init();
	am_hal_sysctrl_fpu_enable();
	am_hal_sysctrl_fpu_stacking_enable(true);

	// Init UART, registers with SDK printf
	uart_init(&uart, UART_INST0);

	am_hal_interrupt_master_enable();

	// Print the banner.
	am_util_stdio_terminal_clear();
	am_util_stdio_printf("Hello World!\r\n\r\n");

	// Initialize spi and select the CS channel
	spi_bus_init(&spi, 0);
	spi_bus_enable(&spi);
	spi_bus_init_device(&spi, &spi_sensor, SPI_CS_1, 2000000u);

	// Initialize sensor
	bmp280_init(&sensor, &spi_sensor);

	// Get ID of sensor
	am_util_stdio_printf("ID: %02X\r\n", bmp280_read_id(&sensor));

	// Test read register function
	uint32_t buffer = 0;
	bmp280_read_register(&sensor, 0xD0, &buffer, 1);
	am_util_stdio_printf("Read Register ID: %02X\r\n", buffer);

	// Test temp compensation
	uint32_t raw_temp = bmp280_get_adc_temp(&sensor);
	am_util_stdio_printf("temperature: %F\r\n", bmp280_compensate_T_double(&sensor, raw_temp));

	// Test pressure compensation
	uint32_t raw_press = bmp280_get_adc_pressure(&sensor);
	am_util_stdio_printf("pressure: %F\r\n", bmp280_compensate_P_double(&sensor, raw_press, raw_temp));

	// Avoid breaking :)
	while(1){

	};
}

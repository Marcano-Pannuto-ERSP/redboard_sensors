// SPDX-License-Identifier: Apache-2.0
// SPDX-FileCopyrightText 2023 Kristin Ebuengan
// SPDX-FileCopyrightText 2023 Melody Gill
// SPDX-FileCopyrightText 2023 Gabriel Marcano

/* 
* This is an edited file of main.c from https://github.com/gemarcano/redboard_lora_example
* which this repo was forked from
*
* Tests reading time from the RTC to reading/writing/erasing from the flash chip.
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

/** Structure representing the BMP280 sensor */
struct BMP280
{
	struct spi *spi;
};

void bmp280_init(struct BMP280 *sensor, struct spi *spi)
{
	sensor->spi = spi;
}

uint8_t BMP280_read_id(struct BMP280 *BMP280)
{
	uint32_t sensorID = 0xD0;
	spi_write_continue(BMP280->spi, &sensorID, 1);
	uint32_t readBuffer = 0;
	spi_read(BMP280->spi, &readBuffer, 1);
	return (uint8_t)readBuffer;
}

// // change these
// uint8_t flash_read_status_register(struct flash *flash)
// {
// 	uint32_t writeBuffer = 0x05;
// 	spi_write_continue(flash->spi, &writeBuffer, 1);
// 	uint32_t readBuffer = 0;
// 	spi_read(flash->spi, &readBuffer, 1);
// 	return (uint8_t)readBuffer;
// }

// void flash_write_enable(struct flash *flash)
// {
// 	uint32_t writeBuffer = 0x06;
// 	spi_write(flash->spi, &writeBuffer, 1);
// }



static struct uart uart;
static struct spi spi;
static struct BMP280 sensor;

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

	// Print the banner.
	am_util_stdio_terminal_clear();
	am_util_stdio_printf("Hello World!\r\n\r\n");

	// Initialize spi and select the CS channel
	spi_init(&spi, 0, 2000000u);
	spi_enable(&spi);

	// Initialize sensor
	bmp280_init(&sensor, &spi);

	// Get ID of sensor
	am_util_stdio_printf("ID: %02X\r\n", BMP280_read_id(&sensor));
	am_util_stdio_printf("test\r\n");
}

// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include <uart.h>
#include <adc.h>
#include <spi.h>
#include <lora.h>
#include <gpio.h>

/** Structure representing the flash chip */
struct flash
{
	struct spi *spi;
};

void flash_init(struct flash *flash, struct spi *spi)
{
	flash->spi = spi;
}

void flash_read_data(struct flash *flash, uint32_t addr, uint32_t *buffer, uint32_t size)
{
	// Write the read command
	// uint8_t toWrite[3];
	// toWrite[0] = addr >> 16 & 0xFF;
	// toWrite[1] = addr >> 8 & 0xFF;
	// toWrite[2] = addr & 0xFF;
	// spi_write(flash->spi, 0x03, toWrite, 3);

	addr &= 0xFFFFFF;
	uint32_t toWrite = addr;
	toWrite += (0x03 << 24);
	spi_write_continue(flash->spi, &toWrite, 4);
	spi_read(flash->spi, buffer, size);
}

static struct uart uart;
static struct spi spi;
static struct flash flash;

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

	// gpio_init(&lora_power, 10, GPIO_MODE_OUTPUT, false);
	//lora_receive_mode(&lora);

	// Initialize spi and select the CS channel
	spi_init(&spi, 0, 2000000u);
	spi_chip_select(&spi, SPI_CS_0);
	spi_enable(&spi);

	// Initialize flash
	flash_init(&flash, &spi);

	// Test flash functions
	int size = 10;
	uint32_t buffer[size];		// this is 4x bigger than necessary
	for (int i = 0; i < size; i++) {
		buffer[i] = -1;
	}
	// print buffer
	for (int i = 0; i < size; i++) {
		am_util_stdio_printf("%d ", buffer[i]);
		am_util_delay_ms(10);
	}
	am_util_stdio_printf("\r\n");

	flash_read_data(&flash, 0x04, buffer, size);

	char* buf = buffer;

	while (1)
	{
		// print buffer
		for (int i = 0; i < size; i++) {
			am_util_stdio_printf("%02X ", (int)buf[i]);
			am_util_delay_ms(10);
		}
		am_util_stdio_printf("\r\n");

		am_util_delay_ms(250);
	}
}

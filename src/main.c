// SPDX-License-Identifier: Apache-2.0
// Copyright: Gabriel Marcano, 2023

#include "am_mcu_apollo.h"
#include "am_bsp.h"
#include "am_util.h"

#include <string.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>

#include <spi.h>
#include <uart.h>


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
	// Write command as least significant bit
	uint32_t toWrite = 0;
	uint32_t* tmpPtr = &toWrite;
	uint8_t* tmp = (uint8_t*) tmpPtr;
	tmp[0] = 0x03;
	tmp[1] = addr >> 16;
	tmp[2] = addr >> 8;
	tmp[3] = addr;

	spi_write_continue(flash->spi, &toWrite, 4);
	spi_read(flash->spi, buffer, size);
}

// Write data from buffer to flash chip
uint8_t flash_page_program(struct flash *flash, uint32_t addr, uint32_t *buffer, uint32_t size)
{
	flash_write_enable(flash);
	uint8_t read = flash_read_status_register(flash);
	uint8_t mask = 0b00000010;
	read = read & mask;
	read = read >> 1;
	if(read == 1){
		uint32_t toWrite = 0;
		uint32_t* tmpPtr = &toWrite;
		uint8_t* tmp = (uint8_t*) tmpPtr;
		tmp[0] = 0x02;
		tmp[1] = addr >> 16;
		tmp[2] = addr >> 8;
		tmp[3] = addr;
		for(int i = 4; i < (size + 4); i++){
			tmp[i] = (uint8_t*)buffer[i-4];
		}
		spi_write(flash->spi, toWrite, size+4);
		return 1;
	}
	else{
		return 0;
	}
}

uint8_t flash_read_status_register(struct flash *flash)
{
	uint32_t writeBuffer = 0x05;
	spi_write_continue(flash->spi, &writeBuffer, 1);
	uint32_t readBuffer = 0;
	spi_read(flash->spi, &readBuffer, 1);
	return (uint8_t)readBuffer;
}

uint32_t flash_read_id(struct flash *flash)
{
	uint32_t writeBuffer = 0x9F;
	spi_write_continue(flash->spi, &writeBuffer, 1);
	uint32_t readBuffer = 0;
	spi_read(flash->spi, &readBuffer, 3);
	return readBuffer;
}

void flash_write_enable(struct flash *flash)
{
	uint32_t writeBuffer = 0x06;
	spi_write(flash->spi, &writeBuffer, 1);
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

	// Initialize spi and select the CS channel
	spi_init(&spi, 0, 2000000u);
	// spi_chip_select(&spi, SPI_CS_0);
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
			am_util_stdio_printf("%02X ", (int) buf[i]);
			am_util_delay_ms(10);
		}
		am_util_stdio_printf("\r\n");

		// // print buffer as integers
		// for (int i = 0; i < size; i++) {
		// 	am_util_stdio_printf("%d ", buffer[i]);
		// 	am_util_delay_ms(10);
		// }
		// am_util_stdio_printf("\r\n");

		am_util_delay_ms(250);
	}
}

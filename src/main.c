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

void BMP280_read_register(struct BMP280 *BMP280, uint32_t addr, uint32_t *buffer, uint32_t size)
{
	addr |= 0x80;
	spi_write_continue(BMP280->spi, &addr, 1);
	spi_read(BMP280->spi, buffer, size);
}

uint32_t BMP280_get_adc_temp(struct BMP280 *BMP280)
{
	// take out of sleep mode
	// set temp oversampling
	uint32_t activate = 0b00100001;

	// write temp sampling to the register
	uint32_t addr = 0xF4 & 0x7F;
	uint32_t buffer = 0;
	uint8_t* ptr = &buffer;
	ptr[0] = addr;
	ptr[1] = activate;
	spi_write(BMP280->spi, &buffer, 2);

	uint32_t tempRegister = 0xFA;
	spi_write_continue(BMP280->spi, &tempRegister, 1);
	uint32_t readBuffer = 0;
	spi_read(BMP280->spi, &readBuffer, 3);
	uint8_t* ptr2 = &readBuffer;
	uint32_t temp = ptr2[2] + (ptr2[1] << 8) + (ptr2[0] << 16);
	temp = temp >> 4;
	return temp;
}

// These functions are from the BMP280 datasheet section 8.1
// They are edited a little bit but the math is the same

// Returns a signed short from the two parameter bytes
// Assuming little endian (although I don't think that matters here)
int16_t bmp280_make_signed_short(uint8_t msb, uint8_t lsb)
{
	int16_t toReturn = (msb << 8) | (lsb & 0xff);
	return toReturn;
}

// >??????? not sure if this is right
uint16_t bmp280_make_unsigned_short(uint8_t msb, uint8_t lsb)
{
	uint16_t toReturn = (msb << 8) | (lsb & 0xff);
	return toReturn;
}

// Possible version where parameter is uint32_t that contains 2 bytes, msb and lsb
// For use in parsing the digs
uint16_t bmp280_unsigned_short_from_buffer(uint32_t* buffer)
{
	uint8_t* ptr = buffer;
	uint8_t msb = ptr[0];
	uint8_t lsb = ptr[1];

	uint16_t toReturn = (msb << 8) | (lsb & 0xff);
	return toReturn;
}

uint32_t bmp280_get_t_fine(struct BMP280 *BMP280, uint32_t raw_temp)
{
	uint32_t dig_T1_buf;
	BMP280_read_register(BMP280, 0x88, &dig_T1_buf, 2);		//0x88 is lsb and 0x89 is msb
	uint16_t dig_T1 = bmp280_unsigned_short_from_buffer(dig_T1_buf);

	am_util_stdio_printf("dig_T1: %u", dig_T1);

	// uint32_t t_fine;
	// double var1, var2;
	// var1 = (((double)raw_temp)/16384.0 - ((double)dig_T1)/1024.0) * ((double)dig_T2);
	// var2 = ((((double)raw_temp)/131072.0 - ((double)dig_T1)/8192.0) *
	// (((double)raw_temp)/131072.0 - ((double) dig_T1)/8192.0)) * ((double)dig_T3);
	// t_fine = (uint32_t)(var1 + var2);
	// return t_fine;
	return -1;
}
/*
// Returns temperature in DegC, double precision. Output value of “51.23” equals 51.23 DegC.
// t_fine carries fine temperature as global value
double bmp280_compensate_T_double(struct BMP280 *BMP280, uint32_t raw_temp)
{
	uint32_t t_fine = bmp280_get_t_fine(BMP280, raw_temp);
	double T = t_fine / 5120.0;
	return T;
}
// Returns pressure in Pa as double. Output value of “96386.2” equals 96386.2 Pa = 963.862 hPa
double bmp280_compensate_P_double(struct BMP280 *BMP280, uint32_t raw_press, uint32_t raw_temp)
{
	uint32_t t_fine = bmp280_get_t_fine(BMP280, raw_temp);

	double var1, var2, p;
	var1 = ((double)t_fine/2.0) - 64000.0;
	var2 = var1 * var1 * ((double)dig_P6) / 32768.0;
	var2 = var2 + var1 * ((double)dig_P5) * 2.0;
	var2 = (var2/4.0)+(((double)dig_P4) * 65536.0);
	var1 = (((double)dig_P3) * var1 * var1 / 524288.0 + ((double)dig_P2) * var1) / 524288.0;
	var1 = (1.0 + var1 / 32768.0)*((double)dig_P1);
	if (var1 == 0.0)
	{
		return 0; // avoid exception caused by division by zero
	}
	p = 1048576.0 - (double)raw_press;
	p = (p - (var2 / 4096.0)) * 6250.0 / var1;
	var1 = ((double)dig_P9) * p * p / 2147483648.0;
	var2 = p * ((double)dig_P8) / 32768.0;
	p = p + (var1 + var2 + ((double)dig_P7)) / 16.0;
	return p;
}
*/

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

	am_hal_interrupt_master_enable();

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
	uint32_t buffer = 0;
	BMP280_read_register(&sensor, 0xD0, &buffer, 1);
	am_util_stdio_printf("Read Register ID: %02X\r\n", buffer);
	uint32_t raw_temp = BMP280_get_adc_temp(&sensor);
	am_util_stdio_printf("Raw Temp: %u\r\n", raw_temp);

	// Test temp compensation
	bmp280_get_t_fine(&sensor, raw_temp);

	while(1){

	};
}

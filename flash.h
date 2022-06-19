#include <hardware/spi.h>
#include <hardware/dma.h>
#include <SPI.h>
// SS:   pin 10
// MOSI: pin 11
// MISO: pin 12
// SCK:  pin 13

// WinBond flash commands
#define WB_WRITE_ENABLE       0x06
#define WB_WRITE_DISABLE      0x04
#define WB_CHIP_ERASE         0xc7
#define _WB_CHIP_ERASE         0x60
#define WB_READ_STATUS_REG_1  0x05
#define WB_READ_DATA          0x03
#define WB_PAGE_PROGRAM       0x02
#define WB_JEDEC_ID           0x9f

/*
================================================================================
Low-Level Device Routines
The functions below perform the lowest-level interactions with the flash device.
They match the timing diagrams of the datahsheet. They are called by wrapper 
functions which provide a little more feedback to the user. I made them stand-
alone functions so they can be re-used. Each function corresponds to a flash
instruction opcode.
================================================================================
*/

/* 
 * not_busy() polls the status bit of the device until it
 * completes the current operation. Most operations finish
 * in a few hundred microseconds or less, but chip erase 
 * may take 500+ms. Finish all operations with this poll.
 *
 * See section 9.2.8 of the datasheet
 */
void not_busy(void) {
  digitalWrite(6, HIGH);  
  digitalWrite(6, LOW);
  SPI.transfer(WB_READ_STATUS_REG_1);       
  while (SPI.transfer(0) & 1) {}; 
  digitalWrite(6, HIGH);  
}

/*
 * See the timing diagram in section 9.2.35 of the
 * data sheet, "Read JEDEC ID (9Fh)".
 */
void _get_jedec_id(byte *b1, byte *b2, byte *b3) {
    
    digitalWrite(6, HIGH);
    digitalWrite(6, LOW);
    SPI.transfer(WB_JEDEC_ID);
    *b1 = SPI.transfer(0); // manufacturer id
    *b2 = SPI.transfer(0); // memory type
    *b3 = SPI.transfer(0); // capacity
    digitalWrite(6, HIGH);
}  

/*
 * See the timing diagram in section 9.2.26 of the
 * data sheet, "Chip Erase (C7h / 06h)". (Note:
 * either opcode works.)
 */
void _chip_erase(void) {
  digitalWrite(6, HIGH);
  digitalWrite(6, LOW);  
  SPI.transfer(WB_WRITE_ENABLE);
  digitalWrite(6, HIGH);
  digitalWrite(6, LOW);  
  SPI.transfer(WB_CHIP_ERASE);
  digitalWrite(6, HIGH);
  /* See notes on rev 2 
  digitalWrite(6, LOW);  
  SPI.transfer(WB_WRITE_DISABLE);
  digitalWrite(6, HIGH);
  */
  not_busy();
}

/*
 * See the timing diagram in section 9.2.10 of the
 * data sheet, "Read Data (03h)".
 */
void _read_page(word page_number, byte *page_buffer) {
  digitalWrite(6, HIGH);
  digitalWrite(6, LOW);
  SPI.transfer(WB_READ_DATA);
  // Construct the 24-bit address from the 16-bit page
  // number and 0x00, since we will read 256 bytes (one
  // page).
  SPI.transfer((page_number >> 8) & 0xFF);
  SPI.transfer((page_number >> 0) & 0xFF);
  SPI.transfer(0);
  for (int i = 0; i < 256; ++i) {
    page_buffer[i] = SPI.transfer(0);
  }
  digitalWrite(6, HIGH);
  not_busy();
}
 
/*
 * See the timing diagram in section 9.2.21 of the
 * data sheet, "Page Program (02h)".
 */
void _write_page(word page_number, byte *page_buffer) {
  digitalWrite(6, HIGH);
  digitalWrite(6, LOW);  
  SPI.transfer(WB_WRITE_ENABLE);
  digitalWrite(6, HIGH);
  digitalWrite(6, LOW);  
  SPI.transfer(WB_PAGE_PROGRAM);
  SPI.transfer((page_number >>  8) & 0xFF);
  SPI.transfer((page_number >>  0) & 0xFF);
  SPI.transfer(0);
  for (int i = 0; i < 256; ++i) {
    SPI.transfer(page_buffer[i]);
  }
  digitalWrite(6, HIGH);
  /* See notes on rev 2
  digitalWrite(6, LOW);  
  SPI.transfer(WB_WRITE_DISABLE);
  digitalWrite(6, HIGH);
  */
  not_busy();
}

// my code 

int flash_send_byte(uint8_t data) {
	return spi_write_blocking(spi0, &data, 1);
}

bool flash_busy(int cs) {

	gpio_put(cs, 1);
	gpio_put(cs, 0);

	flash_send_byte(WB_READ_STATUS_REG_1);
	uint8_t buf;
	spi_read_blocking(spi0, 0, &buf, 1);

	gpio_put(cs, 1);
	
	return (buf&1);

}

bool get_jdec(pin_size_t cs, uint8_t *b1, uint8_t *b2, uint8_t *b3) {
	
	if (flash_busy(cs)) {return 0;}

	gpio_put(cs, 1);
	gpio_put(cs, 0);

	flash_send_byte(WB_JEDEC_ID);

	spi_read_blocking(spi0, 0, b1, 1);
	spi_read_blocking(spi0, 0, b2, 1);
	spi_read_blocking(spi0, 0, b3, 1);
	
	gpio_put(cs, 1);
	
	return 1;

}

bool flash_write_page(int cs, word page_number, uint8_t *buf) {

	while(flash_busy(cs)) {;}

  gpio_put(cs, 1);
  gpio_put(cs, 0);
  
  flash_send_byte(WB_WRITE_ENABLE);

  gpio_put(cs, 1);
  gpio_put(cs, 0);

  flash_send_byte(WB_PAGE_PROGRAM);
  flash_send_byte((page_number>>8) & 0xff);
  flash_send_byte(page_number & 0xff);
  flash_send_byte(0);
    
	spi_write_blocking(spi0, buf, 256);

	gpio_put(cs, 1);

	return 1;

}

bool flash_read_page(int cs, word page_number, uint8_t *buf) {

  while(flash_busy(cs)) {;}

	gpio_put(cs, 1);
	gpio_put(cs, 0);

	flash_send_byte(WB_READ_DATA);
	flash_send_byte((page_number>>8) & 0xff);
  flash_send_byte(page_number & 0xff);
  flash_send_byte(0);

	int err = spi_read_blocking(spi0, 0, buf, 256);

	gpio_put(cs, 1);

	return err > 1 ? 1 : 0;

}

bool flash_erase_chip(int cs) {

	if ( flash_busy(cs) ) {return 0;}

	gpio_put(cs, 1);
	gpio_put(cs, 0);

	flash_send_byte(WB_WRITE_ENABLE);

	gpio_put(cs, 1);
	gpio_put(cs, 0);

	flash_send_byte(WB_CHIP_ERASE);

	gpio_put(cs, 1);

	// while(flash_busy(cs)) {;}
	// gpio_put(cs, 0);

	// flash_send_byte(WB_WRITE_DISABLE);

	// gpio_put(cs, 1);

	return 1;

}

// code re-written for RP2040 c SDK, origional commands and command defines from:
// https://www.instructables.com/How-to-Design-with-Discrete-SPI-Flash-Memory/

#include <hardware/spi.h>

// WinBond flash commands
#define WB_WRITE_ENABLE       0x06
#define WB_WRITE_DISABLE      0x04
#define WB_CHIP_ERASE         0xc7
#define _WB_CHIP_ERASE         0x60
#define WB_READ_STATUS_REG_1  0x05
#define WB_READ_DATA          0x03
#define WB_PAGE_PROGRAM       0x02
#define WB_JEDEC_ID           0x9f

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

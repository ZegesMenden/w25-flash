#include <Arduino.h>
#include <SPI.h>
#include <HW/flash.h>

// most of the SPI interface code here was taken from
// https://www.instructables.com/How-to-Design-with-Discrete-SPI-Flash-Memory/

#define WRITE_ENABLE       0x06
#define WRITE_DISABLE      0x04
#define CHIP_ERASE         0xc7
#define READ_STATUS_REG_1  0x05
#define READ_DATA          0x03
#define PAGE_PROGRAM       0x02
#define JEDEC_ID           0x9f

#define _TOGGLE_CS(x) (digitalWrite(x, HIGH), digitalWrite(x, LOW))

// wait for flash to be done with whatever its doing?
void flash_chip::_NB()
{
    _TOGGLE_CS(_CS_pin);
    SPI.transfer(READ_STATUS_REG_1);
    while (SPI.transfer(0) & 1) {};
    digitalWrite(_CS_pin, HIGH);
}

bool flash_chip::init()
{
    get_id();
    return true;
}

void flash_chip::get_id()
{
    _TOGGLE_CS(_CS_pin);
    SPI.transfer(JEDEC_ID);
    this->_manufacturer_id = SPI.transfer(0); // manufacturer id
    this->_mem_type = SPI.transfer(0); // memory type
    this->_capacity = SPI.transfer(0); // capacity
    digitalWrite(_CS_pin, HIGH);
    _NB();
}

template < class T >
void flash_chip::write_page(uint16_t page, T& data)
{
    uint8_t* ptr = ((uint8_t*)&data);
    uint16_t size = sizeof(ptr);

    _TOGGLE_CS(_CS_pin);

    SPI.transfer(WRITE_ENABLE);

    _TOGGLE_CS(_CS_pin);

    SPI.transfer(PAGE_PROGRAM);
    SPI.transfer((page >> 8) & 0xFF);
    SPI.transfer((page >> 0) & 0xFF);
    SPI.transfer(0);

    for (int i = 0; i < size; ++i) {
        SPI.transfer(ptr[i]);
    }

    digitalWrite(_CS_pin, HIGH);
    _NB();
}

template < class T >
void flash_chip::read_page(uint16_t page, T* ret)
{
    _TOGGLE_CS(_CS_pin);

    SPI.transfer(READ_DATA);
    // Construct the 24-bit address from the 16-bit page
    // number and 0x00, since we will read 256 bytes (one
    // page).

    SPI.transfer((page >> 8) & 0xFF);
    SPI.transfer((page >> 0) & 0xFF);

    SPI.transfer(0);

    for (int i = 0; i < 256; ++i) {
        page_buf[i] = SPI.transfer(0);
    }

    digitalWrite(_CS_pin, HIGH);
    _NB();

    ret = (T*)page_buf;
}


void flash_chip::erase()
{
    // lmao
}
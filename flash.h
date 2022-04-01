#pragma once

#include <Arduino.h>
#include <SPI.h>

class flash_chip
{
    uint8_t _manufacturer_id, _mem_type, _capacity;
    pin_size_t _CS_pin;

    void _NB();

    uint8_t page_buf[256];

public:

    flash_chip(pin_size_t CS) { this->_CS_pin = CS; };

    bool init();
    void get_id();

    void read_byte(uint16_t page, uint8_t offset, uint8_t* ret);

    template <class T>
    void write_page(uint16_t page, T& data);

    template <class T>
    void read_page(uint16_t page, T* ret);

    void erase();
};
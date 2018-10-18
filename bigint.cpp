/*
 * This file is the library for big int on photon
 * This is a strong type limit library. Users are responsible to
 * meet the input parameter's type.
 * Author: Xiaofan Yu
 * Created on: 10/16/2018
 */
#include "bigint.h"

/*
 * No parameter, set every byte to zero
 */ 
BIGINT::BIGINT() {
	for (uint8_t i = 0; i < MAX_INT_BYTE; ++i)
		myinteger[i] = 0;
}

/*
 * Copy to private array in small endian order
 */ 
BIGINT::BIGINT(const uint8_t *p, uint8_t len) {
	uint8_t mylen;
	if (len < MAX_INT_BYTE)
		mylen = len;
	else
		mylen = MAX_INT_BYTE;
	for (uint8_t i = 0; i < mylen; ++i)
		myinteger[i] = p[i];
}

/*
 * convert uint8_t to BIGINT
 */ 
BIGINT::BIGINT(uint8_t num) {
	myinteger[0] = num;
	for (uint8_t i = 1; i < MAX_INT_BYTE; ++i)
		myinteger[i] = 0;
}

/*
 * convert uint16_t to BIGINT
 */ 
BIGINT::BIGINT(uint16_t num) {
	myinteger[0] = (uint8_t)(num & 0xff);
	myinteger[1] = (uint8_t)(num >> 8);
	for (uint8_t i = 2; i < MAX_INT_BYTE; ++i)
		myinteger[i] = 0;
}

/*
 * convert uint16_t to BIGINT
 */ 
BIGINT::BIGINT(uint32_t num) {
	uint8_t i;
	for (i = 0; i < 4; ++i) {
		myinteger[i] = (uint8_t)(num & 0xff);
		num = num >> 8;
	}
	for (; i < MAX_INT_BYTE; ++i)
		myinteger[i] = 0;
}

/*
 * Demise Function
 */
BIGINT::~BIGINT() {}

/*
 * Print the current integer
 */ 
void BIGINT::print_int() {
	for (uint8_t i = 0; i < MAX_INT_BYTE; ++i)
		Serial.print(myinteger[i]);
	Serial.println(" ");
}

/*
 * Get the uint8_t at index i
 */ 
uint8_t BIGINT::get_byte(uint8_t i) {
	if (i >= MAX_INT_BYTE)
		return 0;
	return myinteger[i];
}

/*
 * No parameter, set every byte to zero
 */ 
void BIGINT::change_value() {
	for (uint8_t i = 0; i < MAX_INT_BYTE; ++i)
		myinteger[i] = 0;
}

/*
 * Copy to private array in small endian order
 */ 
void BIGINT::change_value(const uint8_t *p, uint8_t len) {
	uint8_t mylen;
	if (len < MAX_INT_BYTE)
		mylen = len;
	else
		mylen = MAX_INT_BYTE;
	for (uint8_t i = 0; i < mylen; ++i)
		myinteger[i] = p[i];
}

/*
 * convert uint8_t to BIGINT
 */ 
void BIGINT::change_value(uint8_t num) {
	myinteger[0] = num;
	for (uint8_t i = 1; i < MAX_INT_BYTE; ++i)
		myinteger[i] = 0;
}

/*
 * convert uint16_t to BIGINT
 */ 
void BIGINT::change_value(uint16_t num) {
	myinteger[0] = (uint8_t)(num & 0xff);
	myinteger[1] = (uint8_t)num >> 8;
	for (uint8_t i = 2; i < MAX_INT_BYTE; ++i)
		myinteger[i] = 0;
}

/*
 * convert uint16_t to BIGINT
 */ 
void BIGINT::change_value(uint32_t num) {
	uint8_t i;
	for (i = 0; i < 4; ++i) {
		myinteger[i] = (uint8_t)(num & 0xff);
		num = num >> 8;
	}
	for (; i < MAX_INT_BYTE; ++i)
		myinteger[i] = 0;
}

/*
 * Add 8 bit integer to the current big integer.
 * If overflows, return with 1. Else, return with 0.
 */ 
uint8_t BIGINT::add(uint8_t num) {
	uint16_t addon = (uint16_t)num;
	uint16_t temp;
	for (uint8_t i = 0; i < MAX_INT_BYTE && addon > 0; ++i) {
		temp = (uint16_t)myinteger[i] + addon; // impossible to overflow
		myinteger[i] = (uint8_t)(temp & 0xff);
		addon = temp >> 8;
	}
	if (addon > 0)
		return 1; // overflow
	return 0;
}

/*
 * Add 16 bit integer to the current big integer.
 * If overflows, return with 1. Else, return with 0.
 */ 
uint8_t BIGINT::add(uint16_t num) {
	uint32_t addon = (uint32_t)num;
	uint32_t temp;
	for (uint8_t i = 0; i < MAX_INT_BYTE && addon > 0; ++i) {
		temp = (uint32_t)myinteger[i] + addon; // to avoid overflow
		myinteger[i] = (uint8_t)(temp & 0xff);;
		addon = temp >> 8;
	}
	if (addon > 0)
		return 1; // overflow
	return 0;
}

/*
 * Add another big int to the current big integer.
 * If overflows, return with 1. Else, return with 0.
 */ 
uint8_t BIGINT::add(BIGINT num) {
	uint16_t addon = 0;
	uint16_t temp;
	for (uint8_t i = 0; i < MAX_INT_BYTE && addon > 0; ++i) {
		temp = (uint16_t)myinteger[i] + (uint16_t)num.get_byte(i) + addon; // to avoid overflow
		myinteger[i] = (uint8_t)(temp & 0xff);;
		addon = temp >> 8;
	}
	if (addon > 0)
		return 1; // overflow
	return 0;
}

/*
 * Times a 8 bit integer to the current big integer.
 * If overflows, return with 1. Else, return with 0.
 */ 
uint8_t BIGINT::multiply(uint8_t num) {
	uint16_t addon = 0;
	uint16_t temp;
	for (uint8_t i = 0; i < MAX_INT_BYTE && addon > 0; ++i) {
		temp = (uint16_t)myinteger[i] * (uint16_t)num + addon; // impossible to overflow
		myinteger[i] = (uint8_t)(temp & 0xff);
		addon = temp >> 8;
	}
	if (addon > 0)
		return 1; // overflow
	return 0;
}

/*
 * Times a 16 bit integer to the current big integer.
 * If overflows, return with 1. Else, return with 0.
 */ 
uint8_t BIGINT::muliply(uint16_t num) {
	uint32_t addon = 0;
	uint32_t temp;
	for (uint8_t i = 0; i < MAX_INT_BYTE && addon > 0; ++i) {
		temp = (uint32_t)myinteger[i] * (uint32_t)num + addon; // impossible to overflow
		myinteger[i] = (uint8_t)(temp & 0xff);
		addon = temp >> 8;
	}
	if (addon > 0)
		return 1; // overflow
	return 0;
}
/*
 * This file is the library for big int on photon
 * This is a strong type limit library. Users are responsible to
 * meet the input parameter's type.
 * Author: Xiaofan Yu
 * Created on: 10/16/2018
 */
#ifndef __BIGINT_H_
#define __BIGINT_H_

#include "application.h"
#include "spark_wiring_usbserial.h"

#define MAX_INT_BYTE 20
class BIGINT {
private:
	uint8_t myinteger[MAX_INT_BYTE];
public:
	BIGINT();
	BIGINT(const uint8_t *p, uint8_t len);
	BIGINT(uint8_t num);
	BIGINT(uint16_t num);
	BIGINT(uint32_t num);
	BIGINT(const BIGINT &src);
	~BIGINT();

	void print_int() const;
	uint8_t get_byte(uint8_t i) const; 
	void change_value();
	void change_value(const uint8_t *p, uint8_t len);
	void change_value(uint8_t num);
	void change_value(uint16_t num);
	void change_value(uint32_t num);
	void change_value(BIGINT src);
	uint8_t add(uint8_t num);
	uint8_t add(uint16_t num);
	uint8_t add(BIGINT num);
	uint8_t multiply(uint8_t num);
	uint8_t multiply(uint16_t num); 

	BIGINT operator+(uint8_t num);
	BIGINT operator+(uint16_t num);
	BIGINT operator+(BIGINT num);
	BIGINT operator*(uint8_t num);
	BIGINT operator*(uint16_t num);
};

#endif // __BIGINT_H_
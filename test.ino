#include "bigint.h"

const uint8_t p[5] = {244, 243, 244, 244, 224};
const uint8_t p2[20] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
const uint8_t num1 = 55;
const uint16_t num2 = 33345;
const uint32_t num3 = 34355352;
void setup() {
	Serial.begin(9600);

	BIGINT a;
	a.print_int();
	BIGINT b(p, 5);
	b.print_int();
	BIGINT c(num1);
	c.print_int();
	c.change_value(num2);
	c.print_int();
	c.change_value(num3);
	c.print_int();

	Serial.println(c.add((uint8_t)4));
	c.print_int();
	Serial.println(c.add((uint16_t)500));
	c.print_int();
	Serial.println(c.add(b));
	c.print_int();

	BIGINT d(p2, 20);
	Serial.println(d.add((uint16_t)2555));
	d.print_int();
}

void loop() {
	
}
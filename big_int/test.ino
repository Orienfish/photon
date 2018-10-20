#include "bigint.h"

const uint8_t p[5] = {244, 243, 244, 244, 224};
const uint8_t p2[20] = {255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255};
const uint8_t num1 = 55;
const uint16_t num2 = 33345;
const uint32_t num3 = 34355352;
void setup() {
	Serial.begin(9600);

	BIGINT a;
	a.print_int(); // 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
	BIGINT b(p, 5);
	b.print_int(); // 244 243 244 244 224 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
	BIGINT c(num1);
	c.print_int(); // 55 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
	c.change_value(num2);
	c.print_int(); // 65 130 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 
	c.change_value(num3);
	c.print_int(); // 152 56 12 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0

	Serial.println(c.add((uint8_t)4)); // 0
	c.print_int(); // 156 56 12 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
	Serial.println(c.add((uint16_t)500)); // 0
	c.print_int(); // 144 58 12 2 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0
	Serial.println(c.add(b)); // 0
	c.print_int(); // 132 46 1 247 224 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 

	BIGINT d(p2, 20);
	d.print_int(); // 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255 255
	Serial.println(d.add((uint16_t)2)); // 1
	d.print_int(); // 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0

	c.multiply((uint8_t)3);
	c.print_int();
	c.multiply((uint16_t)2445);
	c.print_int();
}

void loop() {
	
}
#include "i2c.h"
#include "simple_indicators.h"

int init_hardware()
{
	leds_init();
	return i2c_init();
}

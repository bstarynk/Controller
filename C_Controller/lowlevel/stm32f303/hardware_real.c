#include "i2c.h"
#include "leds.h"

int init_hardware()
{
	leds_init();
	i2c_init();
	}

#ifndef HARDWARE_SERIAL
#define HARDWARE_SERIAL


// ------------------------------------------------------------------------------------------------
// includes
// ------------------------------------------------------------------------------------------------

#include <stdint.h>
#include <string.h>

// ------------------------------------------------------------------------------------------------
// constant macros
// ------------------------------------------------------------------------------------------------


// ------------------------------------------------------------------------------------------------
// function macros
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// typedefs, structures, unions and enums
// ------------------------------------------------------------------------------------------------

// ------------------------------------------------------------------------------------------------
// public variables
// ------------------------------------------------------------------------------------------------


// ------------------------------------------------------------------------------------------------
// function prototypes
// ------------------------------------------------------------------------------------------------

int hardware_serial_read_data(unsigned char * data, uint16_t data_size);
int hardware_serial_write_data(const unsigned char * data, uint16_t data_size);
int hardware_serial_init(const char * serial_port);

#endif // HARDWARE_SERIAL
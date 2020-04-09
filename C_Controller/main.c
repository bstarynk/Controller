#include "platform.h"

#ifdef POSIX
#include <FreeRTOS.h>
#endif
#include <stdio.h>
#include <string.h>

#include "controller.h"
#include "alarms.h"
#include "ihm_communication.h"
#include "hardware_simulation.h"

#ifndef NDEBUG
#include "unit_tests.h"
#endif

int main(int argc, const char** argv)
{
    if (argc != 1 && argc != 3) {
        printf("Usage: %s [inputFile outputFile]\n", argv[0]);
        return 1;
    }
#ifndef NDEBUG
    if (!unit_tests_passed())
        return -2;
#endif

    init_ihm(argc==3 ? argv[1] : NULL,
             argc==3 ? argv[2] : NULL);

    int result = self_tests();
    // if (result & 0b111111)
    //     return -1;

#ifdef POSIX
    // TODO replace with FreeRTOS code that will schedule cyclic tasks
#endif
    for (long t_ms=0; true; t_ms+=wait_ms(1)) { // 1kHz
        sense_and_compute();
        update_alarms();
        cycle_respiration();
        if (t_ms % 25) { // 40Hz
            send_and_recv();
            if (is_soft_reset_asked() && soft_reset()) { // FIXME Let cycle_respiration handle that
                return 0;
            }
        }
    }
}

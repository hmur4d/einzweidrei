// PA Board - Benchtop spectrometer

#ifndef _HW_PA_H
#define _HW_PA_H

#include "std_includes.h"

#define PA_UART_BAUDRATE 115200
#define PA_UART_DEVICE "/dev/ttyS1"

// The prompt from the PA microcontroller, indicating the end of the command
#ifdef UNIT_TESTS
// For tests, use something that can be entered on Linux...
#define PA_PROMPT_RESPONSE "\n\n>"
#else
#define PA_PROMPT_RESPONSE "\r\n>"
#endif

/**
 * Run a command on the PA board microcontroller
 * @param command The command to run (nul terminated)
 * @param timeout_ms Timeout after waiting this many milliseconds
 * @return The command response string, must be freed. Returns NULL if the port was not open or writing failed.
 */
char *pa_run_command(const char *command, unsigned int timeout_ms);

#endif // _HW_PA_H

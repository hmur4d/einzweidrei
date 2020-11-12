// PA Board - Benchtop spectrometer

#ifndef _HW_PA_H
#define _HW_PA_H

#include "std_includes.h"

#define PA_UART_BAUDRATE B115200
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

/**
 Init the commm with the PA, 
 do a hard reset on PA uC
 open uart
*/
int pa_init();

/**
 return true if response to command "\r\n" is not NULL
*/
bool is_pa_board_responding();

#endif // _HW_PA_H

// Common support code - Benchtop spectrometer

#ifndef _COMMON_H
#define _COMMON_H

#include "std_includes.h"


/**
 * Macros
 */
#define MINIMUM(a, b)  	(((a) < (b)) ? (a) : (b))
#define MAXIMUM(a, b)  	(((a) > (b)) ? (a) : (b))


/**
 * Get the monotonic time, in milliseconds
 */
long long monotonic_ms();

/*******************************************************************************
 * Function:	SystemSnprintfCat()
 * Parameters:	char *__restrict s, size_t n, const char *__restrict format, ...
 * Return:		uint32_t, Number of chars added to the given buffer, or Zero if an error occurs
 * Notes:		Safer replacement for snprintf() to help prevent buffer overruns when concatenating buffers
 ******************************************************************************/
size_t SystemSnprintfCat(char *__restrict s, size_t n, const char *__restrict format, ...);

#endif // _HW_PA_H


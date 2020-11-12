// Common support code - Benchtop spectrometer

#include "std_includes.h"
#include <time.h>


/**
 * Get the monotonic time, in milliseconds
 */
long long monotonic_ms()
{
	struct timespec ts_now = {0};
	clock_gettime(CLOCK_MONOTONIC, &ts_now);
	long long now = ts_now.tv_sec * 1000 + ts_now.tv_nsec / 1000000;
	return now;
}


/*******************************************************************************
 * Function:	SystemSnprintfCat()
 * Parameters:	char *__restrict s, size_t n, const char *__restrict format, ...
 * Return:		uint32_t, Number of chars added to the given buffer, or Zero if an error occurs
 * Notes:		Safer replacement for snprintf() to help prevent buffer overruns when concatenating buffers
 ******************************************************************************/
size_t SystemSnprintfCat(char *__restrict s, size_t n, const char *__restrict format, ...)
{
	va_list 	args;

	va_start(args, format);
	int len = vsnprintf(s, n, format, args);
	va_end(args);

	if (len < 0)
	{
		len = 0;
	}
	else if (((size_t) len) > n)
	{
		len = n;
	}

	return len;
}


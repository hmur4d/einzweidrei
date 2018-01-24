#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include "log.h"

void _log(char* level, char* format, va_list args) {
	//TODO: multithread => locks

	time_t rawtime;
	time(&rawtime);
	struct tm *timeinfo = localtime(&rawtime);

	printf("[%s][%02d/%02d/%04d][%02d:%02d:%02d] ", 
		level, 
		timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	vprintf(format, args);
	printf("\n");

	//TODO: save to file too
}

void log_debug(char* format, ...) {
	if (LOG_LEVEL <= LEVEL_DEBUG) {
		va_list args;
		va_start(args, format);
		_log("DEBUG", format, args);
		va_end(args);
	}
}

void log_info(char* format, ...) {
	if (LOG_LEVEL <= LEVEL_INFO) {
		va_list args;
		va_start(args, format);
		_log("INFO ", format, args);
		va_end(args);
	}
}

void log_warning(char* format, ...) {
	if (LOG_LEVEL <= LEVEL_WARNING) {
		va_list args;
		va_start(args, format);
		_log("WARN ", format, args);
		va_end(args);
	}
}

void log_error(char* format, ...) {
	if (LOG_LEVEL <= LEVEL_ERROR) {
		va_list args;
		va_start(args, format);
		_log("ERROR", format, args);
		va_end(args);
	}
}


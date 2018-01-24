#include <stdarg.h>
#include <stdio.h>
#include "log.h"

void _log(char* prefix, char* format, va_list args) {
	printf("[%s] ", prefix);
	vprintf(format, args);
	printf("\n");

	//TODO: add timestamp
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
		_log("INFO", format, args);
		va_end(args);
	}
}

void log_warning(char* format, ...) {
	if (LOG_LEVEL <= LEVEL_WARNING) {
		va_list args;
		va_start(args, format);
		_log("WARNING", format, args);
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


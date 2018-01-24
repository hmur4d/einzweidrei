#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <semaphore.h> 

#include "log.h"

static sem_t mutex;
static FILE* logfile = NULL;

int log_init() {
	if (sem_init(&mutex, 0, 1) < 0) {
		return -1;
	}

	logfile = fopen(LOG_FILE, "w+");
	if (logfile == NULL) {
		return -1;
	}	

	return 0;
}

int log_close() {
	if (logfile != NULL) {
		fclose(logfile); //TODO test result?
	}

	if (sem_destroy(&mutex) < 0) {
		return -1;
	}

	return 0;
}

/**
 * Really logs the message. Should not be called directly.
 */
void _log(char* level, char* format, va_list args) {
	time_t rawtime;
	time(&rawtime);
	struct tm *timeinfo = localtime(&rawtime);

	sem_wait(&mutex);

	//log to console
	printf("[%s][%02d/%02d/%04d][%02d:%02d:%02d] ", 
		level, 
		timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	vprintf(format, args);
	printf("\n");

	//log to file
	if (logfile != NULL) {
		fprintf(logfile, "[%s][%02d/%02d/%04d][%02d:%02d:%02d] ",
			level,
			timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
		vfprintf(logfile, format, args);
		fprintf(logfile, "\n");
		fflush(logfile);
	}

	sem_post(&mutex);
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


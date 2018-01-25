#include <stdarg.h>
#include <stdio.h>
#include <time.h>
#include <semaphore.h> 

#include "log.h"

void _log_write(FILE* fp, char* level, struct tm* timeinfo, char* format, va_list args);
void _log(char* level, char* format, va_list args);

static sem_t mutex;
static int loglevel = LEVEL_ALL;
static FILE* logfile = NULL;

//--

int log_init(int level, char* logfile_path) {
	loglevel = level;

	if (sem_init(&mutex, 0, 1) < 0) {
		perror("log_init: unable to init log mutex");
		return -1;
	}

	logfile = fopen(logfile_path, "w+");
	if (logfile == NULL) {
		perror("log_init: unable to open log file");
		return -1;
	}	

	return 0;
}

int log_close() {
	if (logfile != NULL && fclose(logfile) != 0) {
		perror("log_close: nable to close log file");
	}

	if (sem_destroy(&mutex) < 0) {
		perror("log_close: unable to destroy log mutex");
		return -1;
	}

	return 0;
}

void log_debug(char* format, ...) {
	if (loglevel <= LEVEL_DEBUG) {
		va_list args;
		va_start(args, format);
		_log("DEBUG", format, args);
		va_end(args);
	}
}

void log_info(char* format, ...) {
	if (loglevel <= LEVEL_INFO) {
		va_list args;
		va_start(args, format);
		_log("INFO ", format, args);
		va_end(args);
	}
}

void log_warning(char* format, ...) {
	if (loglevel <= LEVEL_WARNING) {
		va_list args;
		va_start(args, format);
		_log("WARN ", format, args);
		va_end(args);
	}
}

void log_error(char* format, ...) {
	if (loglevel <= LEVEL_ERROR) {
		va_list args;
		va_start(args, format);
		_log("ERROR", format, args);
		va_end(args);
	}
}


//--

/*
Write the log to stdout and to the log file.
*/
void _log(char* level, char* format, va_list args) {
	time_t rawtime;
	struct tm timeinfo;

	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);

	_log_write(stdout, level, &timeinfo, format, args);
	_log_write(logfile, level, &timeinfo, format, args);
}

/*
Format & write the log message to any file structure.
*/
void _log_write(FILE* fp, char* level, struct tm* timeinfo, char* format, va_list args) {
	sem_wait(&mutex);

	fprintf(fp, "[%s][%02d/%02d/%04d][%02d:%02d:%02d] ",
		level,
		timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
	vfprintf(fp, format, args);
	fprintf(fp, "\n");
	fflush(fp);

	sem_post(&mutex);
}

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <semaphore.h> 

#include "log.h"

static void _log_write(FILE* fp, char* level, const char* srcfile, const char* function, int line, struct tm* timeinfo, int errcode, char* format, va_list args);
static void _log(char* level, const char* srcfile, const char* function, int line, int errcode, char* format, va_list args);

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

void _log_debug(const char* srcfile, const char* function, int line, int errcode, char* format, ...) {
	if (loglevel <= LEVEL_DEBUG) {
		va_list args;
		va_start(args, format);
		_log("DEBUG", srcfile, function, line, errcode, format, args);
		va_end(args);
	}
}

void _log_info(const char* srcfile, const char* function, int line, int errcode, char* format, ...) {
	if (loglevel <= LEVEL_INFO) {
		va_list args;
		va_start(args, format);
		_log("INFO ", srcfile, function, line, errcode, format, args);
		va_end(args);
	}
}

void _log_warning(const char* srcfile, const char* function, int line, int errcode, char* format, ...) {
	if (loglevel <= LEVEL_WARNING) {
		va_list args;
		va_start(args, format);
		_log("WARN ", srcfile, function, line, errcode, format, args);
		va_end(args);
	}
}

void _log_error(const char* srcfile, const char* function, int line, int errcode, char* format, ...) {
	if (loglevel <= LEVEL_ERROR) {
		va_list args;
		va_start(args, format);
		_log("ERROR", srcfile, function, line, errcode, format, args);
		va_end(args);
	}
}

//--

/*
Write the log to stdout and to the log file.
*/
static void _log(char* level, const char* srcfile, const char* function, int line, int errcode, char* format, va_list args) {
	time_t rawtime;
	struct tm timeinfo;

	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);

	//we need to copy the va_list because _log_write is destructive
	//if we don't, the second call to _log_write may crash
	va_list args_stdout, args_file;
	va_copy(args_stdout, args);
	va_copy(args_file, args);

	_log_write(stdout, level, srcfile, function, line, &timeinfo, errcode, format, args_stdout);
	_log_write(logfile, level, srcfile, function, line, &timeinfo, errcode, format, args_file);

	va_end(args_stdout);
	va_end(args_file);
}

/*
Format & write the log message to any file structure.
*/
static void _log_write(FILE* fp, char* level, const char* srcfile, const char* function, int line, struct tm* timeinfo, int errcode, char* format, va_list args) {
	va_list args_copy;
	va_copy(args_copy, args);

	sem_wait(&mutex);

	//[level][day time][file, function():line]
	fprintf(fp, "[%s][%02d/%02d/%04d %02d:%02d:%02d][%s, %s():%d]\n",
		level,
		timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
		srcfile, function, line);
	vfprintf(fp, format, args);
	
	//optionnally, add error code and message from errno
	if (errcode != 0) {
		fprintf(fp, "\nerrno %d: %s", errcode, strerror(errcode));
	}

	fprintf(fp, "\n\n");
	fflush(fp);

	sem_post(&mutex);
}

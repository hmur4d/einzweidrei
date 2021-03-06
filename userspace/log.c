#include "log.h"

static void write_log(const char* level, const char* srcfile, const char* function, int line, int errcode, const char* format, va_list args);
static void write_log_to(FILE* fp, const char* level, const char* srcfile, const char* function, int line, const struct tm* timeinfo, int errcode, const char* format, va_list args);

static bool initialized = false;
static pthread_mutex_t mutex;
static int loglevel = LEVEL_ALL;
static FILE* logfile = NULL;

//--

int log_level_from_name(const char* name, int default_level) {
	if (name == NULL) {
		return default_level;
	} 
	if (strcasecmp("ALL", name) == 0) {
		return LEVEL_ALL;
	}
	if (strcasecmp("DEBUG", name) == 0) {
		return LEVEL_DEBUG;
	}
	if (strcasecmp("INFO", name) == 0) {
		return LEVEL_INFO;
	}
	if (strcasecmp("WARNING", name) == 0) {
		return LEVEL_WARNING;
	}
	if (strcasecmp("ERROR", name) == 0) {
		return LEVEL_ERROR;
	}
	if (strcasecmp("OFF", name) == 0) {
		return LEVEL_OFF;
	}

	printf("log_level_from_name: unknown level %s, using default log level (%d)", name, default_level);
	return default_level;
}

bool log_init(int level, const char* logfile_path) {
	loglevel = level;

	if (pthread_mutex_init(&mutex, NULL) != 0) {
		perror("log_init: unable to init log mutex");
		return false;
	}

	logfile = fopen(logfile_path, "w+");
	if (logfile == NULL) {
		perror("log_init: unable to open log file");
		return false;
	}	

	initialized = true;
	return true;
}

bool log_close() {
	if (!initialized) {
		log_warning("Trying to close log, but it is not initalized!");
		return true;
	}

	if (logfile != NULL && fclose(logfile) != 0) {
		perror("log_close: unable to close log file");
	}

	if (pthread_mutex_destroy(&mutex) != 0) {
		perror("log_close: unable to destroy log mutex");
		return false;
	}

	initialized = false;
	return true;
}

void _log(const char* srcfile, const char* function, int line, int level, const char* levelname, int errcode, const char* format, ...) {
	if (!initialized) {
		printf("_log: trying to log a message but the log isn't initialized!\n");
		printf("[%-5.5s][%s, %s():%d]\n", levelname, srcfile, function, line);
		printf("%s\n\n", format);
		return;
	}

	if (loglevel <= level) {
		va_list args;
		va_start(args, format);
		write_log(levelname, srcfile, function, line, errcode, format, args);
		va_end(args);
	}
}

//--

/*
Write the log to stdout and to the log file. Adds a timestamp.
*/
static void write_log(const char* level, const char* srcfile, const char* function, int line, int errcode, const char* format, va_list args) {
	time_t rawtime;
	struct tm timeinfo;

	time(&rawtime);
	localtime_r(&rawtime, &timeinfo);

	//we need to copy the va_list because _log_write is destructive
	//if we don't, the second call to _log_write may crash
	va_list args_stdout, args_file;
	va_copy(args_stdout, args);
	va_copy(args_file, args);
	
	pthread_mutex_lock(&mutex);
	write_log_to(stdout, level, srcfile, function, line, &timeinfo, errcode, format, args_stdout);
	write_log_to(logfile, level, srcfile, function, line, &timeinfo, errcode, format, args_file);
	pthread_mutex_unlock(&mutex);

	va_end(args_stdout);
	va_end(args_file);
}

/*
Format & write the log message to any file structure.
*/
static void write_log_to(FILE* fp, const char* level, const char* srcfile, const char* function, int line, const struct tm* timeinfo, int errcode, const char* format, va_list args) {

	//[level][day time][file, function():line]
	fprintf(fp, "[%-5.5s][%02d/%02d/%04d %02d:%02d:%02d][%s, %s():%d]\n",
		level,
		timeinfo->tm_mday, timeinfo->tm_mon + 1, timeinfo->tm_year + 1900, timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec,
		srcfile, function, line);
	vfprintf(fp, format, args);
	
	//optionally, add error code and message from errno
	if (errcode != 0) {
		fprintf(fp, "\nerrno %d: %s", errcode, strerror(errcode));
	}

	fprintf(fp, "\n\n");
}

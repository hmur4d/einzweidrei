#ifndef _LOG_H_
#define _LOG_H_

/*
Logging functions.
*/

#include <stdbool.h>
#include <errno.h>

#define LEVEL_ALL		0
#define LEVEL_DEBUG		1
#define LEVEL_INFO		2
#define LEVEL_WARNING	3
#define LEVEL_ERROR		4
#define LEVEL_OFF		5
#define LEVEL_DEFAULT	LEVEL_INFO

int log_level_from_name(const char* name);

//Initialize the logging system. Must be called before any logging is done.
bool log_init(int level, const char* logfile_path);

//Closes the log file. Should be done at program termination.
bool log_close();

//These macros fill in filename, function and line automatically.
//The program should use these.
//The _errno variants add the errno value and description to the end of the message
#define log_debug(...)			_log(__FILE__, __func__, __LINE__, LEVEL_DEBUG, "DEBUG", 0, __VA_ARGS__)
#define log_info(...)			_log(__FILE__, __func__, __LINE__, LEVEL_INFO, "INFO", 0, __VA_ARGS__)
#define log_warning(...)		_log(__FILE__, __func__, __LINE__, LEVEL_WARNING, "WARN", 0, __VA_ARGS__)
#define log_warning_errno(...)	_log(__FILE__, __func__, __LINE__, LEVEL_WARNING, "WARN", errno, __VA_ARGS__)
#define log_error(...)			_log(__FILE__, __func__, __LINE__, LEVEL_ERROR, "ERROR", 0, __VA_ARGS__)
#define log_error_errno(...)	_log(__FILE__, __func__, __LINE__, LEVEL_ERROR, "ERROR", errno, __VA_ARGS__)

//Logs to file and stdout if the level is >= to log level.
//Used by macros, don't call it directly.
void _log(const char* srcfile, const char* function, int line, int level, const char* levelname, int errcode, const char* format, ...);

#endif /* _LOG_H_ */
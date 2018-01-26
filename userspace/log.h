#ifndef _LOG_H_
#define _LOG_H_

#include <errno.h>

#define LEVEL_ALL 0
#define LEVEL_DEBUG 1
#define LEVEL_INFO 2
#define LEVEL_WARNING 3
#define LEVEL_ERROR 4
#define LEVEL_OFF 5

int log_init(int level, char* logfile_path);
int log_close();

//macros that fill in filename, function and line automatically
//the _errno variants add the errno value and description to the end of the message
#define log_debug(...)			_log_debug(__FILE__, __func__, __LINE__, 0, __VA_ARGS__)
#define log_info(...)			_log_info(__FILE__, __func__, __LINE__, 0, __VA_ARGS__)
#define log_warning(...)		_log_warning(__FILE__, __func__, __LINE__, 0, __VA_ARGS__)
#define log_warning_errno(...)	_log_warning(__FILE__, __func__, __LINE__, errno, __VA_ARGS__)
#define log_error(...)			_log_error(__FILE__, __func__, __LINE__, 0, __VA_ARGS__)
#define log_error_errno(...)	_log_error(__FILE__, __func__, __LINE__, errno, __VA_ARGS__)

//functions taking filename, function, and line
void _log_debug(const char* srcfile, const char* function, int line, int errcode, char* format, ...);
void _log_info(const char* srcfile, const char* function, int line, int errcode, char* format, ...);
void _log_warning(const char* srcfile, const char* function, int line, int errcode, char* format, ...);
void _log_error(const char* srcfile, const char* function, int line, int errcode, char* format, ...);

#endif /* _LOG_H_ */
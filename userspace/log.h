#ifndef _LOG_H_
#define _LOG_H_

#define LEVEL_ALL 0
#define LEVEL_DEBUG 1
#define LEVEL_INFO 2
#define LEVEL_WARNING 3
#define LEVEL_ERROR 4
#define LEVEL_OFF 5

#define LOG_LEVEL LEVEL_ALL

void log_debug(char* format, ...);
void log_info(char* format, ...);
void log_warning(char* format, ...);
void log_error(char* format, ...);

#endif /* _LOG_H_ */
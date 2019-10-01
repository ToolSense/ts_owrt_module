#include <syslog.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>
#include "logger.h"

#define LOG_LEVEL_ERROR   0
#define LOG_LEVEL_WARNING 1
#define LOG_LEVEL_MESSAGE 2
#define LOG_LEVEL_DEBUG   3

/*
 * Prefixes for the different logging levels
 */
#define LOG_PREFIX_MESSAGE ""
#define LOG_PREFIX_ERROR   "ERROR"
#define LOG_PREFIX_WARNING "WARNING"
#define LOG_PREFIX_DEBUG   "DEBUG"

/*
 * Logger internal sctructure
 */
struct logger_t {
	int max_log_level;
	int use_stdout;
	FILE* out_file;
	void (*logger_func) (const int level, const char*);
};

/*
 * Global log struct
*/  
static struct logger_t log_global_set;

static const char* LOG_LEVELS[] = { 
					LOG_PREFIX_ERROR,
				    LOG_PREFIX_WARNING,
				    LOG_PREFIX_MESSAGE,
				    LOG_PREFIX_DEBUG };

void print_to_syslog(const int level, const char* message);
void print_to_file(const int level, const char* message);

/*
 * Close remaining file descriptor and reset global params
 */
void cleanup_internal()
{
	if (log_global_set.out_file) {
		if (!log_global_set.use_stdout) {
			fclose(log_global_set.out_file);
		}

		log_global_set.use_stdout = 0;
		log_global_set.out_file = NULL;
	}
}

/*
 * Reset internal state and set syslog as default target
 */ 
void logger_reset_state(void)
{
	log_global_set.max_log_level = LOG_MAX_LEVEL_ERROR_WARNING_MESSAGE;
	cleanup_internal();
	log_global_set.logger_func = print_to_syslog;
}

/*
 * Print to syslog
 */
void print_to_syslog(const int level, const char* message)
{
	// Print to console
	if(level == LOG_LEVEL_MESSAGE)
		printf("%s\n", message);
	else
		fprintf(stdout, "%s: %s\n", LOG_LEVELS[level], message);

	// Print to syslog
	syslog(LOG_INFO, "[%s] %s\n", LOG_LEVELS[level], message);
}

/*
 * Print to file which can be a regular text file or STDOUT "file"
 */
void print_to_file(const int level, const char* message)
{
	int    res;
	struct tm* current_tm;
	time_t time_now;
	char   timeBuffer[30];

	time(&time_now);
	current_tm = localtime(&time_now);
	strftime(timeBuffer, 26, "%d.%m.%y %H:%M:%S", current_tm);

	// Print to console
	if(level == LOG_LEVEL_MESSAGE)
	{
		// Print to console
		printf("%s\n", message);

		// Print to file
		res = fprintf(log_global_set.out_file, "%s %s\n", timeBuffer, message);
	}
	else
	{
		// Print to console
		fprintf(stderr, "%s: %s\n", LOG_LEVELS[level], message);

		// Print to file
		res = fprintf(log_global_set.out_file, "%s [%s] %s\n", timeBuffer, LOG_LEVELS[level], message);
	}

	if (res == -1) {
		print_to_syslog(LOG_LEVEL_ERROR, "Unable to write to log file!");
		return;
	}

	fflush(log_global_set.out_file);
}

/*
 * Init loger
 */
bool logger_init(const char* filename)
{
	if(logger_set_log_file(filename) > 0)
		return false;

	logger_set_log_level(LOG_MAX_LEVEL_ERROR_WARNING_MESSAGE_DEBUG);
	return true;
}

/*
 * Sel log level
 */
void logger_set_log_level(const int level)
{
	log_global_set.max_log_level = level;
}

/*
 * Set log file name
 */
int logger_set_log_file(const char* filename)
{
	cleanup_internal();

	log_global_set.out_file = fopen(filename, "a");

	if (log_global_set.out_file == NULL) {
		log_error(__FILE__, __LINE__, __FUNCTION__, "Failed to open file %s error %s", filename, strerror(errno)); // svv uncomment
		return -1;
	}

	log_global_set.logger_func = print_to_file;

	return 0;
}

/*
 * Set stdout as default
 */
void logger_set_out_stdout()
{
	cleanup_internal();

	log_global_set.use_stdout = 1;
	log_global_set.logger_func = print_to_file;
	log_global_set.out_file = stdout;
}

/*
 * Write buffer to log
 */
void log_generic(const int level, const char* format, va_list args)
{
	char buffer[256];
	vsprintf(buffer, format, args);
	log_global_set.logger_func(level, buffer);
}

/*
 * Write buffer to log with additions
 */
void log_generic_add(const int level, const char* format, va_list args, const char* fileName, const char* funName, int line)
{
	char buffer[256];

	sprintf(buffer, "file: %s line: %d fun: %s ", fileName, line, funName);
	vsprintf(&(buffer[strlen(buffer)]), format, args);
	log_global_set.logger_func(level, buffer);
}

/*
 * Write error log message
 */
void log_error(const char* fileName, int line, const char* funName, char* format, ...)
{
	va_list args;
	va_start(args, format);
	log_generic_add(LOG_LEVEL_ERROR, format, args, fileName, funName, line);
	va_end(args);
}

/*
 * Write warning log message
 */
void log_warning(char *format, ...)
{
	if (log_global_set.max_log_level < LOG_MAX_LEVEL_ERROR_WARNING_MESSAGE) {
		return;
	}

	va_list args;
	va_start(args, format);
	log_generic(LOG_LEVEL_WARNING, format, args);
	va_end(args);
}

/*
 * Write simple log message
 */
void log_info(char *format, ...)
{
	if (log_global_set.max_log_level < LOG_MAX_LEVEL_ERROR_WARNING_MESSAGE) {
		return;
	}

	va_list args;
	va_start(args, format);
	log_generic(LOG_LEVEL_MESSAGE, format, args);
	va_end(args);
}

/*
 * Write debug log message
 */
void log_debug(const char* fileName, int line, const char* funName, char* format, ...)
{
	if (log_global_set.max_log_level <  LOG_MAX_LEVEL_ERROR_WARNING_MESSAGE_DEBUG) {
		return;
	}

	va_list args;
	va_start(args, format);
	log_generic_add(LOG_LEVEL_DEBUG, format, args, fileName, funName, line);
	va_end(args);
}

#ifndef LOGGER_H
#define LOGGER_H

#define LOG_MAX_LEVEL_ERROR 0
#define LOG_MAX_LEVEL_ERROR_WARNING_MESSAGE 1
#define LOG_MAX_LEVEL_ERROR_WARNING_MESSAGE_DEBUG 2

/*
 * Macroses
 */
#define LOG(msg, ...) do {\
                        log_info(msg, ##__VA_ARGS__);\
                    } while (0)

#define LOG_W(msg, ...) do {\
						log_warning(msg, ##__VA_ARGS__);\
                    } while (0)

#define LOG_E(msg, ...) do {\
						log_error(__FILE__, __LINE__, __FUNCTION__, msg, ##__VA_ARGS__);\
                    } while (0)

#define LOG_D(msg, ...) do {\
						log_debug(__FILE__, __LINE__, __FUNCTION__, msg, ##__VA_ARGS__);\
                    } while (0)

/*
 * Init with default log level
 */
bool logger_init(const char* filename);

/*
 * Close logfile
 */
void logger_deinit();

/*
 * Logging methods by levels
 */
void log_error(const char* fileName, int line, const char* funName, char* format, ...);
void log_warning(char* format, ...);
void log_info(char* format, ...);
void log_debug(const char* fileName, int line, const char* funName, char* format, ...);

/*
 * Log level configurator
 * Default is LOG_MAX_LEVEL_ERROR_WARNING_MESSAGE
 */ 
void logger_set_log_level(const int level);

/*
 * Set target type
 * Default is syslog
 */
void logger_reset_state(void);
int  logger_set_log_file(const char* filename);
void logger_set_out_stdout();

#endif // LOGGER_H

#ifndef _SIMPLELOGGER_H_
#define _SIMPLELOGGER_H_


// Simple logger outputs (bit 0..2) - for output flags
extern const int SLO_CONSOLE;
extern const int SLO_FILE;
extern const int SLO_ALL_OUTPUTS; // Default

// Simple logger levels (bit 3..7) - for mask flags
extern const int SLL_FATAL;
extern const int SLL_ERROR;
extern const int SLL_WARNING;
extern const int SLL_INFO;
extern const int SLL_IDLE;
extern const int SLL_ALL_LEVELS; // Default

/*	stdout and stderr mapping, but only if corresponding level is enabled in startup logger
 * 
 * 					SLO_CONSOLE			SLO_FILE
 * 
 * SLL_FATAL		stderr				logger file
 * SLL_ERROR		stderr				logger file
 * SLL_WARNING		stderr				logger file
 * SLL_INFO			stdout				logger file
 * SLL_IDLE			stdout				logger file
 * No level			stdout				logger file
 */

// Simple logger categories (bit 8..31) - for mask flags
extern const int SLC_CHILDINTERACTION;
extern const int SLC_GAMEPLAY;
extern const int SLC_SOCKETHANDLER;
extern const int SLC_SOCKETCOMMUNICATION;
extern const int SLC_PROCESSDISPATCHING;
extern const int SLC_GENERALERRORS;
extern const int SLC_DEBUG;
extern const int SLC_RELEASE;
extern const int SLC_DEBUG_SEM_STATUS;
extern const int SLC_ALL_CATEGORIES; // Default


void startup_logger(const char *logger_tag, int output_flags, int console_mask_flags, int file_mask_flags);
void shutdown_logger();
void log_printf(int mask_flag, const char *pszName, ...);
void log_appendprintf(int mask_flag, const char *pszName, ...);



#endif // _SIMPLELOGGER_H_
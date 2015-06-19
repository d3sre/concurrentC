#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "SimpleLogger.h"


// Simple logger outputs (bit 0..2) - for output flags
const int SLO_CONSOLE = 1 << 0;
const int SLO_FILE = 1 << 1;
const int SLO_ALL_OUTPUTS = 7; // Default

// Simple logger levels (bit 3..7) - for mask flags
const int SLL_FATAL = 1 << 3;
const int SLL_ERROR = 1 << 4;
const int SLL_WARNING = 1 << 5;
const int SLL_INFO = 1 << 6;
const int SLL_IDLE = 1 << 7;
const int SLL_ALL_LEVELS = 248; // Default

/*	stdout and stderr mapping, but only if corresponding level is enabled in startup logger
 * 
 * 					SLO_CONSOLE			SLO_FILE
 * 
 * SLL_FATAL		stderr				logger file & stderr
 * SLL_ERROR		stderr				logger file & stderr
 * SLL_WARNING		stderr				logger file & stderr
 * SLL_INFO			stdout				logger file
 * SLL_IDLE			stdout				logger file
 * No level			stdout				logger file
 */

// Simple logger categories (bit 8..31) - for mask flags
const int SLC_CHILDINTERACTION = 1 << 8;
const int SLC_GAMEPLAY = 1 << 9;
const int SLC_SOCKETHANDLER = 1 << 10;
const int SLC_SOCKETCOMMUNICATION = 1 << 11;
const int SLC_PROCESSDISPATCHING = 1 << 12;
const int SLC_GENERALERRORS = 1 << 13;
const int SLC_DEBUG = 1 << 14;
const int SLC_RELEASE = 1 << 15;
const int SLC_DEBUG_SEM_STATUS = 1 << 16;
const int SLC_ALL_CATEGORIES = 4294967040; // Default


#define LOGGER_TAG_SIZE 256 // Maximum logger tag size
#define INITIAL_ARG_BUFFER_SIZE 1024 // Initial argument buffer size
#define TRUE 1
#define FALSE 0

typedef int bool;

bool g_logger_initialized = FALSE;
char g_logger_tag[256] = "";
int g_output_flags = 0xFFFFFFFF;
int g_console_mask_flags = 0xFFFFFFFF;
int g_file_mask_flags = 0xFFFFFFFF;
FILE *g_logger_fp = NULL;


void adjust_file_name(char *file_name, int length, char search, char replace)
{
	int i;
	
	for (i = 0; i < length; i++)
	{
		if (file_name[i] == search)
			file_name[i] = replace;
	}
}

void startup_logger(const char *logger_tag, int output_flags, int console_mask_flags, int file_mask_flags)
{
	int intro_length;
	int i;
	char intro_message[256] = "";
	char log_file_name[LOGGER_TAG_SIZE + 4] = "";

	/*if (g_logger_initialized)
	{
		fprintf(stderr, "[WARNING] Logger already initialized!\n");
		return;
	}*/

	strncpy(g_logger_tag, logger_tag, LOGGER_TAG_SIZE);
	g_output_flags = output_flags;
	g_console_mask_flags = console_mask_flags;
	g_file_mask_flags = file_mask_flags;

	strncpy(intro_message, "Logging started", 256);
	intro_length = 1 + strnlen(logger_tag, LOGGER_TAG_SIZE) + 2 + strnlen(intro_message, 256);

	if (g_output_flags & SLO_CONSOLE)
	{
//		fprintf(stdout, "\n");
//		for (i = 0; i < intro_length; i++)
//			fprintf(stdout, "=");
//		fprintf(stdout, "\n");
		fprintf(stdout, "[%s] %s", g_logger_tag, intro_message);
		fprintf(stdout, "\n");
//		for (i = 0; i < intro_length; i++)
//			fprintf(stdout, "=");
//		fprintf(stdout, "\n");
//		fprintf(stdout, "\n");
	}
	if (g_output_flags & SLO_FILE)
	{
		snprintf(log_file_name, LOGGER_TAG_SIZE + 4, "%s.log", logger_tag);
		
		adjust_file_name(log_file_name, strnlen(log_file_name, LOGGER_TAG_SIZE + 4), ':', '_');
		adjust_file_name(log_file_name, strnlen(log_file_name, LOGGER_TAG_SIZE + 4), '/', '_');
		adjust_file_name(log_file_name, strnlen(log_file_name, LOGGER_TAG_SIZE + 4), '\\', '_');
		
		g_logger_fp = fopen(log_file_name, "w+");
		if (g_logger_fp == NULL)
		{
			fprintf(stderr, "[FATAL] Logger file not created!\n");
			return;
		}
	
		fprintf(g_logger_fp, "\n");
		for (i = 0; i < intro_length; i++)
			fprintf(g_logger_fp, "=");
		fprintf(g_logger_fp, "\n");
		fprintf(g_logger_fp, "[%s] %s", g_logger_tag, intro_message);
		fprintf(g_logger_fp, "\n");
		for (i = 0; i < intro_length; i++)
			fprintf(g_logger_fp, "=");
		fprintf(g_logger_fp, "\n");
		fprintf(g_logger_fp, "\n");
		
		fflush(g_logger_fp);
	}

	g_logger_initialized = TRUE;
}

void shutdown_logger()
{
	int i;
	int extro_length;
	char extro_message[256] = "";

	strncpy(extro_message, "Logging stopped", 256);
	extro_length = 1 + strnlen(g_logger_tag, LOGGER_TAG_SIZE) + 2 + strnlen(extro_message, 256);

	if (g_output_flags & SLO_CONSOLE)
	{
//		fprintf(stdout, "\n");
//		for (i = 0; i < extro_length; i++)
//			fprintf(stdout, "=");
//		fprintf(stdout, "\n");
		fprintf(stdout, "[%s] %s", g_logger_tag, extro_message);
		fprintf(stdout, "\n");
//		for (i = 0; i < extro_length; i++)
//			fprintf(stdout, "=");
//		fprintf(stdout, "\n");
//		fprintf(stdout, "\n");
	}
	if (g_output_flags & SLO_FILE)
	{
		if (!g_logger_fp)
		{
			fprintf(stderr, "[FATAL] Logger file not existing!\n");
			g_logger_initialized = FALSE;
			return;
		}

		fprintf(g_logger_fp, "\n");
		for (i = 0; i < extro_length; i++)
			fprintf(g_logger_fp, "=");
		fprintf(g_logger_fp, "\n");
		fprintf(g_logger_fp, "[%s] %s", g_logger_tag, extro_message);
		fprintf(g_logger_fp, "\n");
		for (i = 0; i < extro_length; i++)
			fprintf(g_logger_fp, "=");
		fprintf(g_logger_fp, "\n");
		fprintf(g_logger_fp, "\n");

		fflush(g_logger_fp);

		fclose(g_logger_fp);
	}

	g_logger_initialized = FALSE;
}

void do_log_printf(bool append, int mask_flag, const char *arg_buffer, int arg_buffer_length)
{
	char *output_buffer;
	char level_buffer[11] = "";
	int output_length;

	if (!g_logger_initialized)
	{
		fprintf(stderr, "[WARNING] Logger not yet initialized!\n");
		fprintf(stderr, "[WARNING] Text to be logged: %s\n", arg_buffer);
		return;
	}

	if (((g_console_mask_flags & mask_flag) || ((g_file_mask_flags & mask_flag))) == FALSE)
		return;

	if (mask_flag & SLL_FATAL)
		strncpy(level_buffer, " |   FATAL", 11);
	if (mask_flag & SLL_ERROR)
		strncpy(level_buffer, " |   ERROR", 11);
	if (mask_flag & SLL_WARNING)
		strncpy(level_buffer, " | WARNING", 11);
	if (mask_flag & SLL_INFO)
		strncpy(level_buffer, " |    INFO", 11);
	if (mask_flag & SLL_IDLE)
		strncpy(level_buffer, " |    IDLE", 11);

	output_buffer = NULL;
	output_length = 1 + strnlen(g_logger_tag, LOGGER_TAG_SIZE) + 11 + 2 + strnlen(arg_buffer, arg_buffer_length) + 1;

	output_buffer = (char *)malloc(output_length);

	if (!append)
		snprintf(output_buffer, output_length, "[%s%s] %s", g_logger_tag, level_buffer, arg_buffer);
	else
		snprintf(output_buffer, output_length, "%s", arg_buffer);

	if ((g_output_flags & SLO_CONSOLE) && (g_console_mask_flags & mask_flag))
	{
		if ((mask_flag & SLL_FATAL) || (mask_flag & SLL_ERROR) || (mask_flag & SLL_WARNING))
			fprintf(stderr, "%s", output_buffer);
		else
			fprintf(stdout, "%s", output_buffer);
	}
	
	if ((g_output_flags & SLO_FILE) && (g_file_mask_flags & mask_flag))
	{
		if (!g_logger_fp)
		{
			fprintf(stderr, "[FATAL] Logger file not existing!\n");
			g_logger_initialized = FALSE;
			return;
		}

		fprintf(g_logger_fp, "%s", output_buffer);

		fflush(g_logger_fp);
	}

	if (output_buffer)
		free(output_buffer);
}

void log_printf(int mask_flag, const char *text, ...)
{
	va_list arg_list;
	char *arg_buffer;
	int arg_buffer_length;
	int	number_written;
	bool finished;


	arg_buffer_length = INITIAL_ARG_BUFFER_SIZE;
	arg_buffer = NULL;
	finished = TRUE;

	do {
		if (arg_buffer)
			free(arg_buffer);

		arg_buffer = (char *)malloc(arg_buffer_length);

		va_start(arg_list, text);
		number_written = vsnprintf(arg_buffer, arg_buffer_length, text, arg_list);
		va_end(arg_list);

		if (number_written >= arg_buffer_length - 1) {
			arg_buffer_length = arg_buffer_length * 2;
			finished = FALSE;
		}
	} while (!finished);

	if (!arg_buffer)
		return;

	do_log_printf(FALSE, mask_flag, arg_buffer, arg_buffer_length);

	if (arg_buffer)
		free(arg_buffer);
}

void log_appendprintf(int mask_flag, const char *text, ...)
{
	va_list arg_list;
	char *arg_buffer;
	int arg_buffer_length;
	int	number_written;
	bool finished;


	arg_buffer_length = INITIAL_ARG_BUFFER_SIZE;
	arg_buffer = NULL;
	finished = TRUE;

	do {
		if (arg_buffer)
			free(arg_buffer);

		arg_buffer = (char *)malloc(arg_buffer_length);

		va_start(arg_list, text);
		number_written = vsnprintf(arg_buffer, arg_buffer_length, text, arg_list);
		va_end(arg_list);

		if (number_written >= arg_buffer_length - 1) {
			arg_buffer_length = arg_buffer_length * 2;
			finished = FALSE;
		}
	} while (!finished);

	if (!arg_buffer)
		return;

	do_log_printf(TRUE, mask_flag, arg_buffer, arg_buffer_length);

	if (arg_buffer)
		free(arg_buffer);
}


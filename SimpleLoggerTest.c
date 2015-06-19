#include <stdio.h>
#include <stdlib.h>
#include "SimpleLogger.h"


int main(int argc, char **argv)
{
	startup_logger("TESTLOG", SLO_CONSOLE | SLO_FILE, SLL_ALL_LEVELS | SLC_RELEASE, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
	
	log_printf(SLC_RELEASE, "Line1\n");
	log_printf(SLC_RELEASE, "Line2\n");
	log_printf(SLC_RELEASE, "Line3");
	log_appendprintf(SLC_RELEASE, "...3a");
	log_appendprintf(SLC_RELEASE, "...3b\n");
	log_printf(SLC_RELEASE, "Line4\n");
	
	log_printf(SLL_IDLE | SLC_GAMEPLAY | SLC_RELEASE, "Line1\n");
	log_printf(SLL_IDLE | SLC_GAMEPLAY | SLC_RELEASE, "Line2\n");
	log_printf(SLL_IDLE | SLC_GAMEPLAY | SLC_RELEASE, "Line3");
	log_appendprintf(SLL_IDLE | SLC_GAMEPLAY | SLC_RELEASE, "...3a");
	log_appendprintf(SLL_IDLE | SLC_GAMEPLAY | SLC_RELEASE, "...3b\n");
	log_printf(SLL_IDLE | SLC_GAMEPLAY | SLC_RELEASE, "Line4\n");

	log_printf(SLL_INFO | SLC_GAMEPLAY | SLC_RELEASE, "Line1\n");
	log_printf(SLL_INFO | SLC_GAMEPLAY | SLC_RELEASE, "Line2\n");
	log_printf(SLL_INFO | SLC_GAMEPLAY | SLC_RELEASE, "Line3");
	log_appendprintf(SLL_INFO | SLC_GAMEPLAY | SLC_RELEASE, "...3a");
	log_appendprintf(SLL_INFO | SLC_GAMEPLAY | SLC_RELEASE, "...3b\n");
	log_printf(SLL_INFO | SLC_GAMEPLAY | SLC_RELEASE, "Line4\n");

	log_printf(SLL_WARNING | SLC_GAMEPLAY | SLC_RELEASE, "Line1\n");
	log_printf(SLL_WARNING | SLC_GAMEPLAY | SLC_RELEASE, "Line2\n");
	log_printf(SLL_WARNING | SLC_GAMEPLAY | SLC_RELEASE, "Line3");
	log_appendprintf(SLL_WARNING | SLC_GAMEPLAY | SLC_RELEASE, "...3a");
	log_appendprintf(SLL_WARNING | SLC_GAMEPLAY | SLC_RELEASE, "...3b\n");
	log_printf(SLL_WARNING | SLC_GAMEPLAY | SLC_RELEASE, "Line4\n");

	log_printf(SLL_ERROR | SLC_GAMEPLAY, "Line1\n");
	log_printf(SLL_ERROR | SLC_GAMEPLAY, "Line2\n");
	log_printf(SLL_ERROR | SLC_GAMEPLAY, "Line3");
	log_appendprintf(SLL_ERROR | SLC_GAMEPLAY, "...3a");
	log_appendprintf(SLL_ERROR | SLC_GAMEPLAY, "...3b\n");
	log_printf(SLL_ERROR | SLC_GAMEPLAY, "Line4\n");

	log_printf(SLL_FATAL | SLC_GAMEPLAY | SLC_RELEASE, "Line1\n");
	log_printf(SLL_FATAL | SLC_GAMEPLAY | SLC_RELEASE, "Line2\n");
	log_printf(SLL_FATAL | SLC_GAMEPLAY | SLC_RELEASE, "Line3");
	log_appendprintf(SLL_FATAL | SLC_GAMEPLAY | SLC_RELEASE, "...3a");
	log_appendprintf(SLL_FATAL | SLC_GAMEPLAY | SLC_RELEASE, "...3b\n");
	log_printf(SLL_FATAL | SLC_GAMEPLAY | SLC_RELEASE, "Line4\n");
	
	shutdown_logger();

	exit(EXIT_SUCCESS);
}
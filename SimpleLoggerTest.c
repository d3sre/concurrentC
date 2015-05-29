#include <stdio.h>
#include <stdlib.h>
#include "SimpleLogger.h"


int main(int argc, char **argv)
{
	//startup_logger("TESTLOG:1\\2/3", SLO_CONSOLE | SLO_FILE, SLL_INFO | SLL_FATAL | SLC_CATEGORY1 | SLC_CATEGORY3, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
	startup_logger("TESTLOG:1\\2/3", SLO_CONSOLE | SLO_FILE, SLC_CATEGORY3, SLL_ALL_LEVELS | SLC_ALL_CATEGORIES);
	//startup_logger("TESTLOG:1\\2/3", SLO_CONSOLE | SLO_FILE, SLL_INFO | SLL_FATAL | SLC_CATEGORY1 | SLC_CATEGORY3, SLL_INFO | SLL_FATAL | SLC_CATEGORY1 | SLC_CATEGORY3);

	log_printf(SLL_INFO, "Das ist ein INFO log-Eintrag ohne Kategorie.\n");
	log_printf(SLL_WARNING, "Das ist ein WARNING log-Eintrag ohne Kategorie.\n");
	log_printf(SLL_ERROR, "Das ist ein ERROR log-Eintrag ohne Kategorie.\n");
	log_printf(SLL_FATAL, "Das ist ein FATAL log-Eintrag ohne Kategorie.\n");

	log_printf(SLL_INFO | SLC_CAT1_CHILDINTERACTION, "Das ist ein INFO log-Eintrag der Kategorie 1.\n");
	log_printf(SLL_WARNING | SLC_CAT1_CHILDINTERACTION, "Das ist ein WARNING log-Eintrag der Kategorie 1.\n");
	log_printf(SLL_ERROR | SLC_CAT1_CHILDINTERACTION, "Das ist ein ERROR log-Eintrag der Kategorie 1.\n");
	log_printf(SLL_FATAL | SLC_CAT1_CHILDINTERACTION, "Das ist ein FATAL log-Eintrag der Kategorie 1.\n");

	log_printf(SLL_INFO | SLC_CAT3_SOCKETHANDLER, "Das ist ein INFO log-Eintrag der Kategorie 2.\n");
	log_printf(SLL_WARNING | SLC_CAT3_SOCKETHANDLER, "Das ist ein WARNING log-Eintrag der Kategorie 2.\n");
	log_printf(SLL_ERROR | SLC_CAT3_SOCKETHANDLER, "Das ist ein ERROR log-Eintrag der Kategorie 2.\n");
	log_printf(SLL_FATAL | SLC_CAT3_SOCKETHANDLER, "Das ist ein FATAL log-Eintrag der Kategorie 2.\n");

	log_printf(SLL_INFO | SLC_CAT6_GENERALERRORS, "Das ist ein INFO log-Eintrag der Kategorie 3.\n");
	log_printf(SLL_WARNING | SLC_CAT6_GENERALERRORS, "Das ist ein WARNING log-Eintrag der Kategorie 3.\n");
	log_printf(SLL_ERROR | SLC_CAT6_GENERALERRORS, "Das ist ein ERROR log-Eintrag der Kategorie 3.\n");
	log_printf(SLL_FATAL | SLC_CAT6_GENERALERRORS, "Das ist ein FATAL log-Eintrag der Kategorie 3.\n");

	log_printf(SLC_CAT1_CHILDINTERACTION, "Das ist ein log-Eintrag ohne Level der Kategorie 1.\n");
	log_printf(SLL_INFO | SLC_CAT1_CHILDINTERACTION, "Das ist ein INFO log-Eintrag der Kategorie 1 mit verschiedenen Attributen (%s, %d, %f).\n", "Test String", 15, 27.0);

	shutdown_logger();

	exit(EXIT_SUCCESS);
}
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "app.h"
#include "lang.h"

void log_print(char *format, ...)
{
	va_list args;
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	puts("");
}

void log_clear()
{
	// ...	
}

int app_main_window()
{
	printf(T_APP_NAME " version " T_APP_VERSION " - Use at your own risk!\n");

	while (1) {
		printf("> ");
		char input[100];
		char *line = fgets(input, sizeof(input), stdin);
		if (line == NULL) return 0;

		line[strcspn(line, "\n")] = '\0';

		if (line[0] == '\n') {
			continue;
		} else if (!strcmp(line, "help")) {
			puts("Help");
		}
	}
}

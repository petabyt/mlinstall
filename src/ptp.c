#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <libpict.h>

//#define VERBOSE
void ptp_verbose_log(struct PtpRuntime *r, char *fmt, ...) {
#ifdef VERBOSE
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
#endif
}

void ptp_error_log(struct PtpRuntime *r, char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}

__attribute__ ((noreturn))
void ptp_panic(char *fmt, ...) {
	printf("PTP abort: ");
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
	fflush(stdout);
	putchar('\n');
	abort();
}

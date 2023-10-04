#ifndef PLATFORM_H
#define PLATFORM_H

void log_print(char *format, ...);
void log_clear();

int platform_download(char in[], char out[]);

#endif

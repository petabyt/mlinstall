// A simple place to put some platform-specific functions
#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <urlmon.h>
#endif
#include <unistd.h>

int platform_download(char in[], char out[])
{
#ifdef WIN32
	// https://docs.microsoft.com/en-us/previous-versions/windows/internet-explorer/ie-developer/platform-apis/ms775123(v=vs.85)
	int code = URLDownloadToFileA(NULL, in, out, 0, NULL);
	return code != S_OK;
#else
	char command[512];

	snprintf(command, sizeof(command), "curl -L -4 %s --output %s", in, out);

	return system(command);
#endif
}

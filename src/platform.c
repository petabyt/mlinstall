#include <stdio.h>
#include <stdlib.h>

// A simple place to put some platform-specific
// functions

#ifdef WIN32
#include <urlmon.h>
#endif
#include <unistd.h>
int platform_download(char in[], char out[])
{
	// https://docs.microsoft.com/en-us/previous-versions/windows/internet-explorer/ie-developer/platform-apis/ms775123(v=vs.85)
#ifdef WIN32
	int code = URLDownloadToFileA(NULL, in, out, 0, NULL);
	return code != S_OK;
#endif

#ifdef __unix__
	char command[512];

	snprintf(command, sizeof(command), "curl -L -4 %s --output %s", in, out);

	return system(command);
#endif
}

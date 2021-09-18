#include <stdio.h>
#include <stdlib.h>

// Use shell command to pull file
// TODO: Use libcurl
int platform_download(char in[], char out[])
{
	char command[512];

	// Windows 7/10 compatible command
#ifdef WIN32
	snprintf(command, 512, "certutil -urlcache -split -f \"%s\" %s", in, out);
#endif

#ifdef __unix__
	snprintf(command, 512, "curl -L -4 %s --output %s", in, out);
#endif

	char ret = system(command);
	if (ret == -1) {
		return 1;
	}

	return ret;
}

int platform_delete(char file[]) {
	char command[512];
	
	#ifdef __unix__
		snprintf(command, 512, "rm -rf %s", file);
	#endif
	
	#ifdef WIN32
		snprintf(command, 512, "del %s", file);
	#endif

	return system(command);
}

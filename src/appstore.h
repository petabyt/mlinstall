
#ifndef APPSTORE_H
#define APPSTORE_H

#define MAX_FIELD 1024

enum AppStore { APPSTORE_EOF };

struct AppstoreFields {
	char name[MAX_FIELD];
	char website[MAX_FIELD];
	char download[MAX_FIELD];
	char description[MAX_FIELD];
	char author[MAX_FIELD];
};

int appstore_init();
int appstore_next(struct AppstoreFields *fields);
int appstore_close();

int appstore_remove(char name[]);
int appstore_download(char name[], char download[]);
int appstore_getname(char *buffer, char filename[]);

#endif

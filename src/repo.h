#ifndef REPO_H
#define REPO_H

struct ReleaseInfo {
	char *name;

	struct Release {
		char *title;
		char *url;
		char *firmware;
	}release;

	char *forum_url;
	int early_release;
};

static struct ReleaseInfo releases[1] = {
	{
		"1300D",
		{
			"Early build by Critix",
			"https://bitbucket.org/ccritix/magic-lantern-git/downloads/magiclantern-Nightly.2021Jun15.1300D110.zip",
			"1.1.0",
		},
		"https://www.magiclantern.fm/forum/index.php?topic=17969",
		1
	},
};

#endif

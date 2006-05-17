#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

int
main(void)
{
	DIR *dp;

	mkdir("t", 0755);
	creat("t/a", 0600);
	creat("t/b", 0600);
	creat("t/c", 0600);

	if (dp = opendir("t")) {
		char *wasname;
		struct dirent *entry;
		long here;

		/* skip two */
		entry = readdir(dp);
		entry = readdir(dp);
		entry = readdir(dp);
#ifdef DEBUG
		printf("first found: %s\n", entry->d_name);
#endif

		here = telldir(dp);
#ifdef DEBUG
		printf("pos returned by telldir: %d\n", here);
#endif
		entry = readdir(dp);
#ifdef DEBUG
		printf("second found: %s\n", entry->d_name);
#endif

		wasname = strdup(entry->d_name);
		entry = readdir(dp);
#ifdef DEBUG
		printf("third found: %s\n", entry->d_name);
#endif

		seekdir(dp, here);
		entry = readdir(dp);
#ifdef DEBUG
		printf("should be == second: %s\n", entry->d_name);

		printf("pos given to seekdir: %d\n", here);
#else
		assert(strcmp(entry->d_name, wasname) == 0);
#endif
		seekdir(dp, here);
		here = telldir(dp);
#ifdef DEBUG
		printf("pos returned by telldir: %d\n", here);
#endif

		entry = readdir(dp);
#ifdef DEBUG
		printf("should be == second: %s\n", entry->d_name);
#else
		assert(strcmp(entry->d_name, wasname) == 0);
#endif

		seekdir(dp, here);
		entry = readdir(dp);
#ifdef DEBUG
		printf("should be == second: %s\n", entry->d_name);
#else
		assert(strcmp(entry->d_name, wasname) == 0);
#endif

		closedir(dp);
	}
	return 0;
}

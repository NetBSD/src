/*
 * $Id: shlib.c,v 1.1 1993/10/16 21:52:35 pk Exp $
 */

#include <sys/param.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <dirent.h>
#include <a.out.h>

#include "ld.h"

/*
 * Standard directories to search for files specified by -l.
 */
#ifndef STANDARD_SEARCH_DIRS
#define	STANDARD_SEARCH_DIRS	"/usr/lib", "/usr/local/lib"
#endif

char *standard_search_dirs[] = {
	STANDARD_SEARCH_DIRS
};

int n_search_dirs;

void
add_search_dir(name)
	char	*name;
{
	n_search_dirs++;
	search_dirs = (char **)xrealloc(search_dirs,
				n_search_dirs * sizeof(char *));
	search_dirs[n_search_dirs - 1] = strdup(name);
}

void
std_search_dirs()
{
	char	*cp, *ld_path = getenv("LD_LIBRARY_PATH");
	int	i, n;

	if (ld_path != NULL)
		/* Add search paths from LD_LIBRARY_PATH */
		while ((cp = strtok(ld_path, ":")) != NULL) {
			ld_path = NULL;
			add_search_dir(cp);
		}

	/* Append standard search directories */
	n = sizeof standard_search_dirs / sizeof standard_search_dirs[0];
	for (i = 0; i < n; i++)
		add_search_dir(standard_search_dirs[i]);
}

/*
 * Return true if CP points to a valid dewey number.
 * Decode and leave the result in the array DEWEY.
 * Return the number of decoded entries in DEWEY.
 */

#define MAXDEWEY 8

static int
getdewey(dewey, cp)
int	dewey[];
char	*cp;
{
	int	i, n;

	for (n = 0, i = 0; i < MAXDEWEY; i++) {
		if (*cp == '\0')
			break;

		if (*cp == '.') cp++;
		if (!isdigit(*cp))
			return 0;

		dewey[n++] = strtol(cp, &cp, 10);
	}

	return n;
}

/*
 * Search directories for a shared library matching the given
 * major and minor version numbers.
 *
 * MAJOR == -1 && MINOR == -1	--> find highest version
 * MAJOR != -1 && MINOR == -1   --> find highest minor version
 * MAJOR == -1 && MINOR != -1   --> invalid
 * MAJOR != -1 && MINOR != -1   --> find highest micro version
 */

/* Not interested in devices right now... */
#undef major
#undef minor

char *
findshlib(name, majorp, minorp)
char	*name;
int	*majorp, *minorp;
{
	int		dewey[MAXDEWEY];
	int		ndewey;
	int		tmp[MAXDEWEY];
	int		i;
	int		len;
	char		*lname, *path = NULL;
	int		major = *majorp, minor = *minorp;

	len = strlen(name);
	lname = (char *)alloca(len + sizeof("lib"));
	sprintf(lname, "lib%s", name);
	len += 3;

	ndewey = 0;

	for (i = 0; i < n_search_dirs; i++) {
		DIR		*dd = opendir(search_dirs[i]);
		struct dirent	*dp;

		if (dd == NULL)
			continue;

		while ((dp = readdir(dd)) != NULL) {
			int	n, j, might_take_it = 0;

			if (dp->d_namlen < len + 4)
				continue;
			if (strncmp(dp->d_name, lname, len) != 0)
				continue;
			if (strncmp(dp->d_name+len, ".so.", 4) != 0)
				continue;

			if ((n = getdewey(tmp, dp->d_name+len+4)) == 0)
				continue;

			if (major == -1 && minor == -1) {
				might_take_it = 1;
			} else if (major != -1 && minor == -1) {
				if (tmp[0] == major)
					might_take_it = 1;
			} else if (major != -1 && minor != -1) {
				if (tmp[0] == major)
					if (n == 1 || tmp[1] >= minor)
						might_take_it = 1;
			}

			if (!might_take_it)
				continue;

			for (j = 0; j < n; j++) {
				if (j >= ndewey || tmp[j] > dewey[j])
					break;
				if (tmp[j] < dewey[j])
					j = n;
			}

			if (j >= n)
				continue;

			/* We have a better version */
			if (path)
				free(path);
			path = concat(search_dirs[i], "/", dp->d_name);
			bcopy(tmp, dewey, sizeof(dewey));
			ndewey = n;
			*majorp = dewey[0];
			*minorp = dewey[1];
		}
		closedir(dd);
	}

	return path;
}

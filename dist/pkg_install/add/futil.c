/*	$NetBSD: futil.c,v 1.1.1.1 2007/07/16 13:01:45 joerg Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: futil.c,v 1.7 1997/10/08 07:45:39 charnier Exp";
#else
__RCSID("$NetBSD: futil.c,v 1.1.1.1 2007/07/16 13:01:45 joerg Exp $");
#endif
#endif

/*
 * FreeBSD install - a package for the installation and maintainance
 * of non-core utilities.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * Jordan K. Hubbard
 * 18 July 1993
 *
 * Miscellaneous file access utilities.
 *
 */

#if HAVE_ERR_H
#include <err.h>
#endif
#include "lib.h"
#include "add.h"

/*
 * Assuming dir is a desired directory name, make it and all intervening
 * directories necessary.
 */
int
make_hierarchy(char *dir)
{
	char   *cp1, *cp2;
	char   *argv[2];

	argv[0] = dir;
	argv[1] = NULL;

	if (dir[0] == '/')
		cp1 = cp2 = dir + 1;
	else
		cp1 = cp2 = dir;
	while (cp2) {
		if ((cp2 = strchr(cp1, '/')) != NULL)
			*cp2 = '\0';
		if (fexists(dir)) {
			if (!(isdir(dir) || islinktodir(dir)))
				return FAIL;
		} else {
			if (fexec("mkdir", dir, NULL))
				return FAIL;
			apply_perms(NULL, argv, 1);
		}
		/* Put it back */
		if (cp2) {
			*cp2 = '/';
			cp1 = cp2 + 1;
		}
	}
	return SUCCESS;
}

/*
 * Using permission defaults, apply them as necessary
 */
void
apply_perms(char *dir, char **args, int nargs)
{
	char   *cd_to;
	char	owner_group[128];
	const char	**argv;
	int	i;

	argv = malloc((nargs + 4) * sizeof(char *));
	/*
	 * elements 0..2 are set later depending on Mode.
	 * args is a NULL terminated list of file names.
	 * by appending them to argv, argv becomes NULL terminated also.
	 */
	for (i = 0; i <= nargs; i++)
		argv[i + 3] = args[i];

	if (!dir || args[0][0] == '/') /* absolute path? */
		cd_to = "/";
	else
		cd_to = dir;

	if (Mode) {
		argv[0] = CHMOD_CMD;
		argv[1] = "-R";
		argv[2] = Mode;
		if (pfcexec(cd_to, argv[0], argv))
			warnx("couldn't change modes of '%s' ... to '%s'",
			    args[0], Mode);
	}
	if (Owner != NULL && Group != NULL) {
		if (snprintf(owner_group, sizeof(owner_group),
			     "%s:%s", Owner, Group) > sizeof(owner_group)) {
			warnx("'%s:%s' is too long (%lu max)",
			      Owner, Group, (unsigned long) sizeof(owner_group));
			free(argv);
			return;
		}
		argv[0] = CHOWN_CMD;
		argv[1] = "-R";
		argv[2] = owner_group;
		if (pfcexec(cd_to, argv[0], argv))
			warnx("couldn't change owner/group of '%s' ... to '%s:%s'",
				args[0], Owner, Group);
		free(argv);
		return;
	}
	if (Owner != NULL) {
		argv[0] = CHOWN_CMD;
		argv[1] = "-R";
		argv[2] = Owner;
		if (pfcexec(cd_to, argv[0], argv))
			warnx("couldn't change owner of '%s' ... to '%s'",
				args[0], Owner);
		free(argv);

		return;
	}
	if (Group != NULL) {
		argv[0] = CHGRP_CMD;
		argv[1] = "-R";
		argv[2] = Group;
		if (pfcexec(cd_to, argv[0], argv))
			warnx("couldn't change group of '%s' ... to '%s'",
				args[0], Group);
	}
	free(argv);
}

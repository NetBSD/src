/*	$NetBSD: futil.c,v 1.13 2003/09/23 09:36:04 wiz Exp $	*/

#include <sys/cdefs.h>
#ifndef lint
#if 0
static const char *rcsid = "from FreeBSD Id: futil.c,v 1.7 1997/10/08 07:45:39 charnier Exp";
#else
__RCSID("$NetBSD: futil.c,v 1.13 2003/09/23 09:36:04 wiz Exp $");
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

#include <err.h>
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
			apply_perms(NULL, dir);
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
apply_perms(char *dir, char *arg)
{
	char   *cd_to;
	char	owner_group[128];

	if (!dir || *arg == '/')/* absolute path? */
		cd_to = "/";
	else
		cd_to = dir;

	if (Mode)
		if (fcexec(cd_to, CHMOD_CMD, "-R", Mode, arg, NULL))
			warnx("couldn't change modes of '%s' to '%s'", arg,
			    Mode);
	if (Owner != NULL && Group != NULL) {
		if (snprintf(owner_group, sizeof(owner_group),
			     "%s:%s", Owner, Group) > sizeof(owner_group)) {
			warnx("'%s:%s' is too long (%d max)",
			      Owner, Group, sizeof(owner_group));
			return;
		}
		if (fcexec(cd_to, CHOWN_CMD, "-R", owner_group, arg))
			warnx("couldn't change owner/group of '%s' to '%s:%s'",
			    arg, Owner, Group);
		return;
	}
	if (Owner != NULL) {
		if (fcexec(cd_to, CHOWN_CMD, "-R", Owner, arg, NULL))
			warnx("couldn't change owner of '%s' to '%s'", arg,
			    Owner);
		return;
	}
	if (Group != NULL) {
		if (fcexec(cd_to, CHGRP_CMD, "-R", Group, arg, NULL))
			warnx("couldn't change group of '%s' to '%s'", arg,
			    Group);
	}
}

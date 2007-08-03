/* $NetBSD: delete.h,v 1.1.1.2.14.2 2007/08/03 13:58:21 joerg Exp $ */

/* from FreeBSD Id: delete.h,v 1.4 1997/02/22 16:09:35 peter Exp */

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
 * Include and define various things wanted by the delete command.
 *
 */

#ifndef _INST_DELETE_H_INCLUDE
#define _INST_DELETE_H_INCLUDE

extern char *Prefix;
extern char *ProgramPath;
extern Boolean NoDeleteFiles;
extern Boolean NoDeInstall;
extern Boolean CleanDirs;
extern Boolean Force;
extern Boolean Recurse_up;
extern Boolean Recurse_down;
extern lpkg_head_t pkgs;

int     pkg_perform(lpkg_head_t *);

#endif				/* _INST_DELETE_H_INCLUDE */

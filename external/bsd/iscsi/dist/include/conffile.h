/* $NetBSD: conffile.h,v 1.3 2009/06/30 02:44:52 agc Exp $ */

/*
 * Copyright © 2006 Alistair Crooks.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote
 *    products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#ifndef CONFFILE_H_
#define CONFFILE_H_	1

/* split routines */

#include <sys/param.h>

#include <stdio.h>
#include <time.h>

#include "defs.h"

DEFINE_ARRAY(strv_t, char *);

/* this struct describes a configuration file */
typedef struct conffile_t {
	FILE		*fp;			/* in-core file pointer */
	char		 name[MAXPATHLEN];	/* name of file */
	int	 	 lineno;		/* current line number */
	int		 readonly;		/* nonzero if file is readonly */
	const char	*sep;			/* set of separators */
	const char	*comment;		/* set of comment characters */
} conffile_t;

/* this struct describes an entry in the configuration file */
typedef struct ent_t {
	char		buf[BUFSIZ];		/* buffer with entry contents */
	strv_t		sv;			/* split up string dynamic array */
} ent_t;

int conffile_open(conffile_t *, const char *, const char *, const char *, const char *);
int conffile_split(conffile_t *, ent_t *, char *);
int conffile_getent(conffile_t *, ent_t *);
int conffile_get_by_field(conffile_t *, ent_t *, int, char *);
int conffile_putent(conffile_t *, int, char *, char *);
int conffile_get_lineno(conffile_t *);
char *conffile_get_name(conffile_t *);
void conffile_printent(ent_t *);
void conffile_close(conffile_t *);

#ifndef PREFIX
#define PREFIX	""
#endif

#ifndef SYSCONFDIR
#define SYSCONFDIR	"/etc"
#endif

#define _PATH_ISCSI_ETC		SYSCONFDIR "/iscsi/"

#define _PATH_ISCSI_PASSWD	PREFIX _PATH_ISCSI_ETC "auths"
#define	_PATH_ISCSI_TARGETS	PREFIX _PATH_ISCSI_ETC "targets"
#define	_PATH_ISCSI_PID_FILE	"/var/run/iscsi-target.pid"

#endif

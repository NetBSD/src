/*
 * Copyright (c) 1993 Paul Kranenburg
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Paul Kranenburg.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software withough specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *	$Id: dl.c,v 1.1 1993/10/17 00:45:46 pk Exp $
 */

#include <sys/param.h>
#include <sys/types.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <a.out.h>

#include <link.h>

static struct ld_entry	*entry;

static int
dlinit()
{
	struct link_dynamic	*dp;
	struct exec		*hp;

	hp = (struct exec *)(__LDPGSZ);
	if (N_BADMAG(*hp)) {
		fprintf(stderr, "Bad magic\n");
		errno = EFTYPE;
		return -1;
	}

	if (!N_IS_DYNAMIC(*hp)) {
		fprintf(stderr, "Not dynamic\n");
		errno = EFTYPE;
		return -1;
	}

	dp = (struct link_dynamic *)N_DATADDR(*hp);
	if (dp->ld_version != LD_VERSION_BSD) {
		fprintf(stderr, "Unsupported link_dynamic version\n");
		errno = EOPNOTSUPP;
		return -1;
	}

	entry = dp->ld_entry;
}

int
dlopen(name, mode)
char	*name;
int	mode;
{
	if (entry == NULL && dlinit() == -1)
		return -1;

	return (entry->dlopen)(name, mode);
}

int
dlclose(fd)
int	fd;
{
	if (entry == NULL)
		dlinit();

	return (entry->dlclose)(fd);
}

int
dlsym(fd, name)
int	fd;
char	*name;
{
	if (entry == NULL && dlinit() == -1)
		return -1;

	return (entry->dlsym)(fd, name);
}

/*	$NetBSD: pmon.c,v 1.3 2003/07/14 22:57:47 lukem Exp $	*/

/*-
 * Copyright (c) 2001 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
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
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include <sys/cdefs.h>
__KERNEL_RCSID(0, "$NetBSD: pmon.c,v 1.3 2003/07/14 22:57:47 lukem Exp $");

#include <sys/param.h>
#include <sys/systm.h>

#include <machine/pmon.h>

static char **environ;

/*
 * pmon_init:
 *
 *	Initialize the PMON interface.
 */
void
pmon_init(char *envp[])
{

	if (environ == NULL)
		environ = envp;
#ifdef PMON_DEBUG
	printf("pmon_init: environ = %p (%p)\n", environ, *environ);
#endif
}

/*
 * pmon_getenv:
 *
 *	Fetch an environment variable.
 */
const char *
pmon_getenv(const char *var)
{
	const char *rv = NULL;
	char **env = environ;
	int i;

	i = strlen(var);

	while (*env != NULL) {
#ifdef PMON_DEBUG
		printf("pmon_getenv: %s\n", *env);
#endif
		if (strncasecmp(var, *env, i) == 0 &&
		    (*env)[i] == '=') {
			rv = &(*env)[i + 1];
			break;
		}
		env++;
	}

	return (rv);
}

/*	$NetBSD: pam_debug_log.c,v 1.3.34.1 2012/04/17 00:05:29 yamt Exp $	*/

/*-
 * Copyright 2001 Mark R V Murray
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
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifdef __FreeBSD__
__FBSDID("$FreeBSD: src/lib/libpam/libpam/pam_debug_log.c,v 1.8 2002/04/14 16:44:04 des Exp $");
#else
__RCSID("$NetBSD: pam_debug_log.c,v 1.3.34.1 2012/04/17 00:05:29 yamt Exp $");
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <security/pam_appl.h>
#include <security/openpam.h>
#include <security/pam_mod_misc.h>

/* Print a verbose error, including the function name and a
 * cleaned up filename.
 */
void
_pam_verbose_error(pam_handle_t *pamh, int flags,
    const char *file, const char *function, const char *format, ...)
{
	va_list ap;
	char *msg;
	int rv;
	const char *modname, *period;

	if ((flags & PAM_SILENT) || openpam_get_option(pamh, "no_warn"))
		return;
	modname = strrchr(file, '/');
	if (modname == NULL)
		modname = file;
	period = strchr(modname, '.');
	if (period == NULL)
		period = strchr(modname, '\0');
	va_start(ap, format);
	rv = vasprintf(&msg, format, ap);
	va_end(ap);
	if (rv < 0)
		return;
	pam_error(pamh, "%.*s: %s: %s\n", (int)(ssize_t)(period - modname),
	    modname, function, msg);
	free(msg);
}

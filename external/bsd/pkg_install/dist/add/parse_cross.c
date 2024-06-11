/*	$NetBSD: parse_cross.c,v 1.1.1.1 2024/06/11 09:15:38 wiz Exp $	*/

#if HAVE_CONFIG_H
#include "config.h"
#endif
#include <nbcompat.h>
#if HAVE_SYS_CDEFS_H
#include <sys/cdefs.h>
#endif
__RCSID("$NetBSD: parse_cross.c,v 1.1.1.1 2024/06/11 09:15:38 wiz Exp $");

#if HAVE_ERR_H
#include <err.h>
#endif
#include "lib.h"
#include "add.h"

/*
 * ${OPSYS}/${MACHINE_ARCH} ${OS_VERSION}
 *
 * or just
 *
 * ${MACHINE_ARCH}
 */
void
parse_cross(const char *text, char **machine_arch, char **opsys,
    char **os_version)
{
	static const char safeset[] = /* XXX */
	    "abcdefghijklmnopqrstuvwxyz"
	    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
	    "0123456789"
	    "-._";
	char *copy = xstrdup(text);
	char *p = copy, *q, *r;

	/*
	 * If there's no /, treat it as a single MACHINE_ARCH.
	 */
	if ((q = strchr(p, '/')) == NULL) {
		*machine_arch = copy;
		*opsys = NULL;
		*os_version = NULL;
	} else {
		/*
		 * NUL-terminate at the slash so p := text[0..slash)
		 * is the OPSYS.
		 */
		*q++ = '\0';

		/*
		 * If there's no SPC, fail.
		 */
		if (*(r = strchr(q, ' ')) == '\0') {
			goto fail;
		}

		/*
		 * NUL-terminate at the space so
		 *
		 *	q := text(slash..space)
		 *
		 * is the MACHINE_ARCH.
		 */
		*r++ = '\0';

		/*
		 * The rest is already NUL-terminated, so
		 *
		 *	r := text(space..NUL)
		 *
		 * is the OS_VERSION.
		 */
		*opsys = p;
		*machine_arch = q;
		*os_version = r;
	}

	/*
	 * Verify that MACHINE_ARCH, and, if specified, OPSYS and
	 * OS_VERSION lie within the safe set, so we can reserve large
	 * amounts of the space of inputs for additional syntax.
	 * Ideally we would impose more constraints here with a
	 * regular expression to restrict the space even more, but
	 * this'll do for now.
	 */
	if ((*machine_arch)[strspn(*machine_arch, safeset)] != '\0') {
		goto fail;
	}
	if (*opsys != NULL && (*opsys)[strspn(*opsys, safeset)] != '\0') {
		goto fail;
	}
	if (*os_version != NULL &&
	    (*os_version)[strspn(*os_version, safeset)] != '\0') {
		goto fail;
	}
	return;

fail:	errx(1, "Invalid -m argument: ${OPSYS}/${MACHINE_ARCH} ${OS_VERSION}");
}

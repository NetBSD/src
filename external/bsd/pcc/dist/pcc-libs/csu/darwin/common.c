/*	$Id: common.c,v 1.1.1.1.6.2 2008/10/19 22:40:21 haad Exp $	*/
/*-
 * Copyright (c) 2008 Gregory McGarry <g.mcgarry@ieee.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include "macho.h"

static char *
_strrchr(char *p, int ch)
{
	char *save;

	for (save = NULL;; ++p) {
		if (*p == ch)
			save = (char *)p;
		if (!*p)
			return(save);
	}
	/* NOTREACHED */
}

static int
_strcmp(char *s1, char *s2)
{
	while (*s1 == *s2++)
		if (*s1++ == 0)
			return 0;
	s2--;
	return ((long)*s1 - (long)*s2);
}

#ifdef PROFILE
static void
_mcleanup(void)
{
	monitor(0, 0, 0, 0, 0);
}
#endif

extern struct mach_header _mh_execute_header;

static void
_helper(int init)
{
	struct mach_header *hdr = &_mh_execute_header;
	char *ptr = (char *)(hdr + 1);
	int i, j, n;
	struct segment_command *segp;
	struct section *secp;
	void (*func)(void);
	void **addr;

        for (i = 0; i < (int)hdr->ncmds; i++, ptr += segp->cmdsize) {
                segp = (struct segment_command *)ptr;
                if (segp->cmd != LC_SEGMENT || segp->nsects == 0)
                        continue;
                secp = (struct section *)(segp + 1);
                for (j = 0; j < (int)segp->nsects; j++, secp++) {
                        if (init && _strcmp(secp->sectname, "__constructor") != 0)
				continue;
			if (!init && _strcmp(secp->sectname, "__destructor") != 0)
				continue;
			n = secp->size / 4;
			addr = (void **)secp->addr;
			while (n--) {
				func = *addr++;
				(*func)();
			}
                }
        }
}

void
_init(void)
{
	_helper(1);
}

void
_fini(void)
{
	_helper(0);
}

#ifdef DYNAMIC

void
_dyld_init(void)
{
        void (*init)(void);
        _dyld_func_lookup("__dyld_make_delayed_module_initializer_calls",
            (void *)&init);
	if (init)
	        init();
}

void
_dyld_fini(void)
{
        void (*term)(void);
        _dyld_func_lookup("__dyld_mod_term_funcs", (void *)&term);
        if (term)
                term();
}

#endif

IDENT("$Id: common.c,v 1.1.1.1.6.2 2008/10/19 22:40:21 haad Exp $");

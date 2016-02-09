/*	Id	*/	
/*	$NetBSD: wflag.c,v 1.1.1.1 2016/02/09 20:29:13 plunky Exp $	*/

/*-
 * Copyright (c) 2014 Iain Hibbert.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <string.h>

#include "driver.h"

static struct {
	const char *	name;
	unsigned int	flag;
} Warns[] = {
    { "error",				W_CCOM | W_CXXCOM		},
    { "truncate",			W_CCOM | W_CXXCOM		},
    { "strict-prototypes",		W_CCOM | W_CXXCOM		},
    { "missing-prototypes",		W_CCOM | W_CXXCOM		},
    { "implicit-int",			W_CCOM | W_CXXCOM | W_ALL	},
    { "implicit-function-declaration",	W_CCOM | W_CXXCOM | W_ALL	},
    { "shadow",				W_CCOM | W_CXXCOM		},
    { "pointer-sign",			W_CCOM | W_CXXCOM | W_ALL	},
    { "sign-compare",			W_CCOM | W_CXXCOM		},
    { "system-headers",			W_CPP				},
    { "unknown-pragmas",		W_CCOM | W_CXXCOM | W_ALL	},
    { "unreachable-code",		W_CCOM | W_CXXCOM		},
};

static int
Wset(unsigned int match)
{
	size_t i;

	for (i = 0; i < ARRAYLEN(Warns); i++) {
		if ((Warns[i].flag & match) == match)
			Warns[i].flag |= W_SET;
	}

	return 1;
}

int
Wflag(const char *str)
{
	size_t i;
	int set, err;

	if (strcmp("all", str) == 0)
		return Wset(W_ALL);
	if (strcmp("extra", str) == 0)
		return Wset(W_EXTRA);
	if (strcmp("W", str) == 0)
		return Wset(0);

	set = 1;
	err = 0;
	if (strcmp("error", str) != 0) {
		if (strncmp("no-", str, 3) == 0) {
			str += 3;
			set = 0;
		}

		if (strncmp("error=", str, 6) == 0) {
			str += 6;
			err = 1;
		}
	}

	for (i = 0; i < ARRAYLEN(Warns); i++) {
		if (strcmp(Warns[i].name, str) == 0) {
			if (set)
				Warns[i].flag |= W_SET;
			else
				Warns[i].flag &= ~W_SET;
				
			if (err)
				Warns[i].flag |= W_ERR;
			else
				Warns[i].flag &= ~W_ERR;

			return 1;
		}
	}

	return 0;
}

void
Wflag_add(list_t *l, unsigned int match)
{
	size_t i;

	for (i = 0; i < ARRAYLEN(Warns); i++) {
		if ((Warns[i].set || Warns[i].err)
		    && (match & Warns[i].flag)) {
			list_add(l, "-W%s%s%s",
			    (Warns[i].flag & W_SET ? "" : "no-"),
			    (Warns[i].flag & W_ERR ? "error=" : ""),
			    Warns[i].name);
		}
	}
}

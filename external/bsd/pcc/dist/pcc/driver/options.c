/*	Id	*/	
/*	$NetBSD: options.c,v 1.1.1.1 2016/02/09 20:29:13 plunky Exp $	*/

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

#include <ctype.h>
#include <string.h>

#include "driver.h"

/*
 * list of options we understand. The list should be sorted by
 * initial char, then by increasing match
 */
struct options {
	const char *	name;
	unsigned int	type;
	void		(*func)(int, char *);
	void *		addr;
};

/* option types */
#define FLAG		(0)		/* match name exactly */
#define STR(n)		((n) & 0xff)	/* match n chars (or all if n = 0) */
#define ARG(n)		(STR(n)|VAL)	/* match, and require an argument */
#define VAL		0x0100		/* arg value is required */

static void ignore(int, char *);	/* do nothing */

static void opt_set(int, char *);	/* set option */
static void opt_set2(int, char *);	/* set option */
static void opt_clr(int, char *);	/* clear option */
static void opt_add(int, char *);	/* add option to list */

static void str_set(int, char *);	/* set string option */
static void str_add(int, char *);	/* add string to list */
static void str_expand(int, char *);	/* expand string to list */

static void arg_add(int, char *);	/* add option and argument to list */

static void str_Olevel(int, char *);	/* handle -Olevel */
static void str_glevel(int, char *);	/* handle -glevel */

static struct options options[] = {
    { "-param",			ARG(0),		ignore,		NULL		},
    { "-sysroot=",		STR(9),		str_set,	&opt.sysroot	},
    { "-version",		FLAG,		opt_set,	&opt.version,	},
    { "B",			ARG(1),		arg_add,	&opt.prefix	},
    { "C",			FLAG,		opt_set,	&opt.C		},
    { "CC",			FLAG,		opt_set2,	&opt.C		},
    { "D",			ARG(1),		arg_add,	&opt.DIU	},
    { "E",			FLAG,		opt_set,	&opt.E		},
    { "I",			ARG(1),		arg_add,	&opt.DIU	},
    { "L",			ARG(1),		arg_add,	&opt.ldargs	},
    { "M",			FLAG,		opt_set,	&opt.M		},
    { "O",			STR(1),		str_Olevel,	NULL		},
    { "P",			FLAG,		opt_set,	&opt.P		},
    { "S",			FLAG,		opt_set,	&opt.S		},
    { "U",			ARG(1),		arg_add,	&opt.DIU	},
    { "Wa,",			STR(3),		str_expand,	&opt.Wa		},
    { "Wl,",			STR(3),		str_expand,	&opt.Wl		},
    { "Wp,",			STR(3),		str_expand,	&opt.Wp		},
    { "W",			STR(1),		str_Wflag,	NULL		},	// XX
    { "X",			FLAG,		opt_set,	&opt.savetemps	},
    { "c",			FLAG,		opt_set,	&opt.c		},
    { "compatibility-version",	FLAG,		ignore,		NULL		},
    { "current-version",	FLAG,		ignore,		NULL		},
    { "dynamiclib",		FLAG,		opt_set,	&opt.dynamiclib	},
    { "ffreestanding",		FLAG,		opt_clr,	&opt.hosted	},
    { "fno-freestanding",	FLAG,		opt_set,	&opt.hosted	},
    { "fsigned-char",		FLAG,		opt_clr,	&opt.uchar	},
    { "fno-signed-char",	FLAG,		opt_set,	&opt.uchar	},
    { "funsigned-char",		FLAG,		opt_set,	&opt.uchar	},
    { "fno-unsigned-char",	FLAG,		opt_clr,	&opt.uchar	},
    { "fpic",			FLAG,		opt_set,	&opt.pic	},
    { "fno-pic",		FLAG,		opt_clr,	&opt.pic	},
    { "fPIC",			FLAG,		opt_set,	&opt.pic	},
    { "fno-PIC",		FLAG,		opt_clr,	&opt.pic	},
    { "fstack-protector",	FLAG,		opt_set,	&opt.ssp	},
    { "fno-stack-protector",	FLAG,		opt_clr,	&opt.ssp	},
    { "fstack-protector-all",	FLAG,		opt_set,	&opt.ssp	},
    { "fno-stack-protector-all",FLAG,		opt_clr,	&opt.ssp	},
    { "f",			STR(1),		ignore,		NULL		},
    { "g",			STR(1),		str_glevel,	NULL		},
    { "idirafter",		FLAG,		opt_set,	&opt.idirafter	},
    { "isystem",		ARG(0),		str_set,	&opt.isystem	},
    { "include",		ARG(0),		str_add,	&opt.include	},
    { "install-name",		ARG(0),		ignore,		NULL		},	// XX
    { "iquote",			ARG(0),		ignore,		NULL		},	// XX
    { "isysroot",		ARG(0),		str_set,	&opt.isysroot	},
    { "i",			STR(1),		opt_add,	&opt.ldargs	},
    { "k",			FLAG,		opt_set,	&opt.pic	},
    { "l",			ARG(1),		arg_add,	&opt.ldargs	},
#ifdef mach_arm
    { "mbig-endian",		FLAG,		opt_set,	&opt.bigendian	},
    { "mlittle-endian",		FLAG,		opt_clr,	&opt.bigendian	},
#endif
#ifdef mach_amd64 || mach_i386
    { "m32",			FLAG,		opt_set,	&opt.m32	},
    { "m64",			FLAG,		opt_clr,	&opt.m32	},
#endif
    { "m",			ARG(1),		ignore,		NULL		},	// XX
    { "nostdinc",		FLAG,		opt_clr,	&opt.stdinc	},
    { "nostdinc++",		FLAG,		opt_clr,	&opt.cxxinc	},
    { "nostdlib",		FLAG,		opt_clr,	&opt.stdlib	},
    { "nostartfiles",		FLAG,		opt_clr,	&opt.startfiles	},
    { "nodefaultlibs",		FLAG,		opt_clr,	&opt.defaultlibs},
    { "n",			STR(1),		arg_add,	&opt.ldargs	},	// XX
    { "o",			ARG(1),		str_set,	&opt.outfile	},
    { "pedantic",		FLAG,		opt_set,	&opt.pedantic	},
    { "p",			FLAG,		opt_set,	&opt.profile	},
    { "pg",			FLAG,		opt_set,	&opt.profile	},
    { "pipe",			FLAG,		opt_set,	&opt.pipe	},
    { "print-prog-name=",	STR(15),	ignore,		NULL		},	// XX
    { "print-multi-os-directory", FLAG,		opt_set,	&opt.print	},
    { "pthread",		FLAG,		opt_set,	&opt.pthread	},
    { "r",			FLAG,		opt_set,	&opt.r		},
    { "save-temps",		FLAG,		opt_set,	&opt.savetemps	},
    { "shared",			FLAG,		opt_set,	&opt.shared	},
    { "static",			FLAG,		opt_set,	&opt.ldstatic	},
    { "symbolic",		FLAG,		opt_set,	&opt.symbolic	},
    { "std=",			STR(4),		str_standard,	NULL		},	// XX
    { "t",			FLAG,		opt_set,	&opt.traditional},
    { "traditional",		FLAG,		opt_set,	&opt.traditional},
    { "undef",			FLAG,		opt_clr,	&opt.stddef	},
    { "v",			FLAG,		opt_set,	&opt.verbose	},
    { "x",			ARG(1),		arg_language,	NULL		},	// XX
};

/* do nothing */
static void
ignore(int n, char *arg)
{
	fprintf(stderr, "option `-%s' ignored\n", options[n].name);
}

/* set option */
static void
opt_set(int n, char *arg)
{

	*(int *)(options[n].addr) = 1;
}

/* set option=2 */
static void
opt_set2(int n, char *arg)
{

	*(int *)(options[n].addr) = 2;
}

/* clear option */
static void
opt_clr(int n, char *arg)
{

	*(int *)(options[n].addr) = 0;
}

/* add option to list */
static void
opt_add(int n, char *arg)
{
	list_t *l = options[n].addr;

	list_add(l, "-%s", options[n].name);
}

/* set string option */
static void
str_set(int n, char *arg)
{
	char **p = options[n].addr;

	if (*p != NULL)
		warning("option `-%s' was already set", options[n].name);

	*p = arg;
}

/* add string to list */
static void
str_add(int n, char *arg)
{
	list_t *l = options[n].addr;

	list_add(l, arg);
}

/* expand comma separated string to list */
static void
str_expand(int n, char *arg)
{
	list_t *l = options[n].addr;
	char *next;

	while (arg != NULL) {
		next = strchr(arg, ',');
		if (next != NULL)
			*next++ = '\0';

		list_add(l, arg);
		arg = next;
	}
}

/* add option and argument to list */
static void
arg_add(int n, char *arg)
{
	list_t *l = options[n].addr;

	list_add(l, "-%s", options[n].name);
	list_add(l, arg);
}

/* handle -Olevel */
static void
str_Olevel(int n, char *arg)
{

	if (arg[0] == '\0')
		opt.optim++;
	else if (arg[1] == '\0' && isdigit((unsigned char)arg[1]))
		opt.optim = arg[1] - '0';
	else if (arg[1] == '\0' && arg[1] == 's')
		opt.optim = 1;	/* optimize for space only */
	else
		warning("unknown optimization `-O%s'", arg);
}

/* handle -glevel */
static void
str_glevel(int n, char *arg)
{

	if (arg[0] == '\0')
		opt.debug++;
	else if (arg[1] == '\0' && isdigit((unsigned char)arg[1]))
		opt.debug = arg[1] - '0';
	else
		warning("unknown debug `-g%s'", arg);
}

/*
 * return true if av1 is used
 */
bool
add_option(char *av0, char *av1)
{
	unsigned int n;
	char *p;
	size_t i

	i = 0;
	p = av0 + 1;	/* skip initial `-' */

	while (i < ARRAYLEN(options) && *p != options[i].name[0])
		i++;

	while (i < ARRAYLEN(options) && *p == options[i].name[0]) {
		n = STR(options[i].type);
		if ((n > 0 && strncmp(options[i].name, p, n) == 0)
		    || (n == 0 && strcmp(options[i].name, p) == 0)) {
			if ((options[i].type & VAL)
			    && (n == 0 || *(p += n) == '\0')
			    && (p = av1) == NULL) {
				warning("option `%s' requires a value", av0);
				return false;
			}

			options[i].func(i, p);
			return (p == av1);
		}
		i++;
	}

	warning("unknown option `%s'", av0);
	return false;
}

/*	$NetBSD: ulimit.c,v 1.3 1995/04/11 03:17:45 christos Exp $	*/

/*
 * ulimit builtin
 *
 * This code, originally by Doug Gwyn, Doug Kingston, Eric Gisin, and
 * Michael Rendell was ripped from pdksh 5.0.8 and hacked for use with
 * ash by J.T. Conklin.
 *
 * Public domain.
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>

#include "shell.h"
#include "options.h"
#include "output.h"

struct limits {
	const char *name;
	int	cmd;
	int	factor;	/* multiply by to get rlim_{cur,max} values */
	char	option;
};

static const struct limits limits[] = {
#ifdef RLIMIT_CPU
	{ "time(seconds)",		RLIMIT_CPU,	   1, 't' },
#endif
#ifdef RLIMIT_FSIZE
	{ "file(blocks)",		RLIMIT_FSIZE,	 512, 'f' },
#endif
#ifdef RLIMIT_DATA
	{ "data(kbytes)",		RLIMIT_DATA,	1024, 'd' },
#endif
#ifdef RLIMIT_STACK
	{ "stack(kbytes)",		RLIMIT_STACK,	1024, 's' },
#endif
#ifdef  RLIMIT_CORE
	{ "coredump(blocks)",		RLIMIT_CORE,	 512, 'c' },
#endif
#ifdef RLIMIT_RSS
	{ "memory(kbytes)",		RLIMIT_RSS,	1024, 'm' },
#endif
#ifdef RLIMIT_MEMLOCK
	{ "locked memory(kbytes)",	RLIMIT_MEMLOCK, 1024, 'l' },
#endif
#ifdef RLIMIT_NPROC
	{ "process(processes)",		RLIMIT_NPROC,      1, 'p' },
#endif
#ifdef RLIMIT_NOFILE
	{ "nofiles(descriptors)",	RLIMIT_NOFILE,     1, 'n' },
#endif
#ifdef RLIMIT_VMEM
	{ "vmemory(kbytes)",		RLIMIT_VMEM,	1024, 'v' },
#endif
#ifdef RLIMIT_SWAP
	{ "swap(kbytes)",		RLIMIT_SWAP,	1024, 'w' },
#endif
	{ (char *) 0,			0,		   0,  '\0' }
};

int
ulimitcmd(argc, argv)
	int argc;
	char **argv;
{
	register int	c;
	quad_t val;
	enum { SOFT = 0x1, HARD = 0x2 }
			how = SOFT | HARD;
	const struct limits	*l;
	int		set, all = 0;
	int		optc, what;
	struct rlimit	limit;

	what = 'f';
	while ((optc = nextopt("HSatfdsmcnpl")) != '\0')
		switch (optc) {
		case 'H':
			how = HARD;
			break;
		case 'S':
			how = SOFT;
			break;
		case 'a':
			all = 1;
			break;
		default:
			what = optc;
		}

	for (l = limits; l->name && l->option != what; l++)
		;
	if (!l->name)
		error("ulimit: internal error (%c)\n", what);

	set = *argptr ? 1 : 0;
	if (set) {
		char *p = *argptr;

		if (all || argptr[1])
			error("ulimit: too many arguments\n");
		if (strcmp(p, "unlimited") == 0)
			val = RLIM_INFINITY;
		else {
			val = (quad_t) 0;

			while ((c = *p++) >= '0' && c <= '9')
			{
				val = (val * 10) + (long)(c - '0');
				if (val < (quad_t) 0)
					break;
			}
			if (c)
				error("ulimit: bad number\n");
			val *= l->factor;
		}
	}
	if (all) {
		for (l = limits; l->name; l++) {
			getrlimit(l->cmd, &limit);
			if (how & SOFT)
				val = limit.rlim_cur;
			else if (how & HARD)
				val = limit.rlim_max;

			out1fmt("%-20s ", l->name);
			if (val == RLIM_INFINITY)
				out1fmt("unlimited\n");
			else
			{
				val /= l->factor;
				out1fmt("%ld\n", (long) val);
			}
		}
		return 0;
	}

	getrlimit(l->cmd, &limit);
	if (set) {
		if (how & SOFT)
			limit.rlim_cur = val;
		if (how & HARD)
			limit.rlim_max = val;
		if (setrlimit(l->cmd, &limit) < 0)
			error("ulimit: bad limit\n");
	} else {
		if (how & SOFT)
			val = limit.rlim_cur;
		else if (how & HARD)
			val = limit.rlim_max;
	}

	if (!set) {
		if (val == RLIM_INFINITY)
			out1fmt("unlimited\n");
		else
		{
			val /= l->factor;
			out1fmt("%ld\n", (long) val);
		}
	}
	return 0;
}

/*	$NetBSD: kdump.c,v 1.115.2.1 2013/01/23 00:06:38 yamt Exp $	*/

/*-
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include <sys/cdefs.h>
#ifndef lint
__COPYRIGHT("@(#) Copyright (c) 1988, 1993\
 The Regents of the University of California.  All rights reserved.");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)kdump.c	8.4 (Berkeley) 4/28/95";
#else
__RCSID("$NetBSD: kdump.c,v 1.115.2.1 2013/01/23 00:06:38 yamt Exp $");
#endif
#endif /* not lint */

#include <sys/param.h>
#include <sys/file.h>
#define _KMEMUSER        /* To get the pseudo errors defined */
#include <sys/errno.h>
#undef _KMEMUSER
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/ktrace.h>
#include <sys/ioctl.h>
#include <sys/ptrace.h>
#include <sys/socket.h>

#include <ctype.h>
#include <err.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>
#include <util.h>

#include "ktrace.h"
#include "setemul.h"

#include <sys/syscall.h>

static int timestamp, decimal, plain, tail, maxdata = -1, numeric;
static int word_size = 0;
static pid_t do_pid = -1;
static const char *tracefile = NULL;
static struct ktr_header ktr_header;
static int emul_changed = 0;

#define eqs(s1, s2)	(strcmp((s1), (s2)) == 0)
#define small(v)	(((long)(v) >= 0) && ((long)(v) < 10))

static const char * const ptrace_ops[] = {
	PT_STRINGS
};

#ifdef PT_MACHDEP_STRINGS
static const char * const ptrace_machdep_ops[] = { PT_MACHDEP_STRINGS };
#endif

static const char * const linux_ptrace_ops[] = {
	"PTRACE_TRACEME",
	"PTRACE_PEEKTEXT", "PTRACE_PEEKDATA", "PTRACE_PEEKUSER",
	"PTRACE_POKETEXT", "PTRACE_POKEDATA", "PTRACE_POKEUSER",
	"PTRACE_CONT", "PTRACE_KILL", "PTRACE_SINGLESTEP",
	NULL, NULL,
	"PTRACE_GETREGS", "PTRACE_SETREGS", "PTRACE_GETFPREGS",
	"PTRACE_SETFPREGS", "PTRACE_ATTACH", "PTRACE_DETACH",
	NULL, NULL, NULL, NULL, NULL, NULL,
	"PTRACE_SYSCALL",
};

int	main(int, char **);
static int	fread_tail(void *, size_t, size_t);
static int	dumpheader(struct ktr_header *);
static void	output_long(u_long, int);
static void	ioctldecode(u_long);
static void	ktrsyscall(struct ktr_syscall *);
static void	ktrsysret(struct ktr_sysret *, int);
static void	ktrnamei(char *, int);
static void	ktremul(char *, int, int);
static void	ktrgenio(struct ktr_genio *, int);
static void	ktrpsig(void *, int);
static void	ktrcsw(struct ktr_csw *);
static void	ktruser(struct ktr_user *, int);
static void	ktrmib(int *, int);
static void	ktrexecfd(struct ktr_execfd *);
static void	usage(void) __dead;
static void	eprint(int);
static void	rprint(register_t);
static const char *signame(long, int);
static void hexdump_buf(const void *, int, int);
static void visdump_buf(const void *, int, int);

int
main(int argc, char **argv)
{
	int ch, ktrlen, size;
	void *m;
	int trpoints = 0;
	int trset = 0;
	const char *emul_name = "netbsd";
	int col;
	char *cp;

	setprogname(argv[0]);

	if (strcmp(getprogname(), "ioctlname") == 0) {
		int i;

		while ((ch = getopt(argc, argv, "e:")) != -1)
			switch (ch) {
			case 'e':
				emul_name = optarg;
				break;
			default:
				usage();
				break;
			}
		setemul(emul_name, 0, 0);
		argv += optind;
		argc -= optind;

		if (argc < 1)
			usage();

		for (i = 0; i < argc; i++) {
			ioctldecode(strtoul(argv[i], NULL, 0));
			(void)putchar('\n');
		}
		return 0;
	}
		
	while ((ch = getopt(argc, argv, "e:f:dlm:Nnp:RTt:xX:")) != -1) {
		switch (ch) {
		case 'e':
			emul_name = strdup(optarg); /* it's safer to copy it */
			break;
		case 'f':
			tracefile = optarg;
			break;
		case 'd':
			decimal = 1;
			break;
		case 'l':
			tail = 1;
			break;
		case 'p':
			do_pid = strtoul(optarg, &cp, 0);
			if (*cp != 0)
				errx(1,"invalid number %s", optarg);
			break;
		case 'm':
			maxdata = strtoul(optarg, &cp, 0);
			if (*cp != 0)
				errx(1,"invalid number %s", optarg);
			break;
		case 'N':
			numeric++;
			break;
		case 'n':
			plain++;
			break;
		case 'R':
			timestamp = 2;	/* relative timestamp */
			break;
		case 'T':
			timestamp = 1;
			break;
		case 't':
			trset = 1;
			trpoints = getpoints(trpoints, optarg);
			if (trpoints < 0)
				errx(1, "unknown trace point in %s", optarg);
			break;
		case 'x':
			word_size = 1;
			break;
		case 'X':
			word_size = strtoul(optarg, &cp, 0);
			if (*cp != 0 || word_size & (word_size - 1) ||
			    word_size > 16 || word_size <= 0)
				errx(1, "argument to -X must be "
				    "1, 2, 4, 8 or 16");
			break;
		default:
			usage();
		}
	}
	argv += optind;
	argc -= optind;

	if (!trset)
		trpoints = ALL_POINTS;

	if (tracefile == NULL) {
		if (argc == 1) {
			tracefile = argv[0];
			argv++;
			argc--;
		} else
			tracefile = DEF_TRACEFILE;
	}

	if (argc > 0)
		usage();

	setemul(emul_name, 0, 0);

	m = malloc(size = 1024);
	if (m == NULL)
		errx(1, "malloc: %s", strerror(ENOMEM));
	if (!freopen(tracefile, "r", stdin))
		err(1, "%s", tracefile);
	while (fread_tail(&ktr_header, sizeof(struct ktr_header), 1)) {
		if (trpoints & (1 << ktr_header.ktr_type) &&
		    (do_pid == -1 || ktr_header.ktr_pid == do_pid))
			col = dumpheader(&ktr_header);
		else
			col = -1;
		if ((ktrlen = ktr_header.ktr_len) < 0)
			errx(1, "bogus length 0x%x", ktrlen);
		if (ktrlen > size) {
			while (ktrlen > size)
				size *= 2;
			m = realloc(m, size);
			if (m == NULL)
				errx(1, "realloc: %s", strerror(ENOMEM));
		}
		if (ktrlen && fread_tail(m, ktrlen, 1) == 0)
			errx(1, "data too short");
		if (col == -1)
			continue;

		/* update context to match currently processed record */
		ectx_sanify(ktr_header.ktr_pid);

		switch (ktr_header.ktr_type) {
		case KTR_SYSCALL:
			ktrsyscall(m);
			break;
		case KTR_SYSRET:
			ktrsysret(m, ktrlen);
			break;
		case KTR_NAMEI:
			ktrnamei(m, ktrlen);
			break;
		case KTR_GENIO:
			ktrgenio(m, ktrlen);
			break;
		case KTR_PSIG:
			ktrpsig(m, ktrlen);
			break;
		case KTR_CSW:
			ktrcsw(m);
			break;
		case KTR_EMUL:
			ktremul(m, ktrlen, size);
			break;
		case KTR_USER:
			ktruser(m, ktrlen);
			break;
		case KTR_EXEC_ARG:
		case KTR_EXEC_ENV:
			visdump_buf(m, ktrlen, col);
			break;
		case KTR_EXEC_FD:
			ktrexecfd(m);
			break;
		case KTR_MIB:
			ktrmib(m, ktrlen);
			break;
		default:
			putchar('\n');
			hexdump_buf(m, ktrlen, word_size ? word_size : 1);
		}
		if (tail)
			(void)fflush(stdout);
	}
	return (0);
}

static int
fread_tail(void *buf, size_t num, size_t size)
{
	int i;

	while ((i = fread(buf, size, num, stdin)) == 0 && tail) {
		(void)sleep(1);
		clearerr(stdin);
	}
	return (i);
}

static int
dumpheader(struct ktr_header *kth)
{
	char unknown[64];
	const char *type;
	union holdtime {
		struct timeval tv;
		struct timespec ts;
	};
	static union holdtime prevtime;
	union holdtime temp;
	int col;

	switch (kth->ktr_type) {
	case KTR_SYSCALL:
		type = "CALL";
		break;
	case KTR_SYSRET:
		type = "RET ";
		break;
	case KTR_NAMEI:
		type = "NAMI";
		break;
	case KTR_GENIO:
		type = "GIO ";
		break;
	case KTR_PSIG:
		type = "PSIG";
		break;
	case KTR_CSW:
		type = "CSW ";
		break;
	case KTR_EMUL:
		type = "EMUL";
		break;
	case KTR_USER:
		type = "MISC";
		break;
	case KTR_EXEC_ENV:
		type = "ENV";
		break;
	case KTR_EXEC_ARG:
		type = "ARG";
		break;
	case KTR_EXEC_FD:
		type = "FD";
		break;
	case KTR_SAUPCALL:
		type = "SAU";
		break;
	case KTR_MIB:
		type = "MIB";
		break;
	default:
		(void)snprintf(unknown, sizeof(unknown), "UNKNOWN(%d)",
		    kth->ktr_type);
		type = unknown;
	}

	col = printf("%6d ", kth->ktr_pid);
	if (kth->ktr_version > KTRFACv0)
		col += printf("%6d ", kth->ktr_lid);
	col += printf("%-8.*s ", MAXCOMLEN, kth->ktr_comm);
	if (timestamp) {
		(void)&prevtime;
		if (timestamp == 2) {
			switch (kth->ktr_version) {
			case KTRFAC_VERSION(KTRFACv0):
				if (prevtime.tv.tv_sec == 0)
					temp.tv.tv_sec = temp.tv.tv_usec = 0;
				else
					timersub(&kth->ktr_otv,
					    &prevtime.tv, &temp.tv);
				prevtime.tv.tv_sec = kth->ktr_otv.tv_sec;
				prevtime.tv.tv_usec = kth->ktr_otv.tv_usec;
				break;
			case KTRFAC_VERSION(KTRFACv1):
				if (prevtime.ts.tv_sec == 0)
					temp.ts.tv_sec = temp.ts.tv_nsec = 0;
				else
					timespecsub(&kth->ktr_ots,
					    &prevtime.ts, &temp.ts);
				prevtime.ts.tv_sec = kth->ktr_ots.tv_sec;
				prevtime.ts.tv_nsec = kth->ktr_ots.tv_nsec;
				break;
			case KTRFAC_VERSION(KTRFACv2):
				if (prevtime.ts.tv_sec == 0)
					temp.ts.tv_sec = temp.ts.tv_nsec = 0;
				else
					timespecsub(&kth->ktr_ts,
					    &prevtime.ts, &temp.ts);
				prevtime.ts.tv_sec = kth->ktr_ts.tv_sec;
				prevtime.ts.tv_nsec = kth->ktr_ts.tv_nsec;
				break;
			default:
				goto badversion;
			}
		} else {
			switch (kth->ktr_version) {
			case KTRFAC_VERSION(KTRFACv0):
				temp.tv.tv_sec = kth->ktr_otv.tv_sec;
				temp.tv.tv_usec = kth->ktr_otv.tv_usec;
				break;
			case KTRFAC_VERSION(KTRFACv1):
				temp.ts.tv_sec = kth->ktr_ots.tv_sec;
				temp.ts.tv_nsec = kth->ktr_ots.tv_nsec;
				break;
			case KTRFAC_VERSION(KTRFACv2):
				temp.ts.tv_sec = kth->ktr_ts.tv_sec;
				temp.ts.tv_nsec = kth->ktr_ts.tv_nsec;
				break;
			default:
			badversion:
				err(1, "Unsupported ktrace version %x\n",
				    kth->ktr_version);
			}
		}
		if (kth->ktr_version == KTRFACv0)
			col += printf("%lld.%06ld ",
			    (long long)temp.tv.tv_sec, (long)temp.tv.tv_usec);
		else
			col += printf("%lld.%09ld ",
			    (long long)temp.ts.tv_sec, (long)temp.ts.tv_nsec);
	}
	col += printf("%-4s  ", type);
	return col;
}

static void
output_long(u_long it, int as_x)
{
	if (cur_emul->flags & EMUL_FLAG_NETBSD32)
		printf(as_x ? "%#x" : "%d", (u_int)it);
	else
		printf(as_x ? "%#lx" : "%ld", it);
}

static void
ioctldecode(u_long cmd)
{
	char dirbuf[4], *dir = dirbuf;
	int c;

	if (cmd & IOC_IN)
		*dir++ = 'W';
	if (cmd & IOC_OUT)
		*dir++ = 'R';
	*dir = '\0';

	c = (cmd >> 8) & 0xff;
	if (isprint(c))
		printf("_IO%s('%c',", dirbuf, c);
	else
		printf("_IO%s(0x%02x,", dirbuf, c);
	output_long(cmd & 0xff, decimal == 0);
	if ((cmd & IOC_VOID) == 0) {
		putchar(',');
		output_long(IOCPARM_LEN(cmd), decimal == 0);
	}
	putchar(')');
}

static void
ktrsyscall(struct ktr_syscall *ktr)
{
	int argcount;
	const struct emulation *emul = cur_emul;
	register_t *ap;
	char c;
	const char *cp;
	const char *sys_name;

	argcount = ktr->ktr_argsize / sizeof (*ap);

	emul_changed = 0;

	if (numeric ||
	    ((ktr->ktr_code >= emul->nsysnames || ktr->ktr_code < 0))) {
		sys_name = "?";
		(void)printf("[%d]", ktr->ktr_code);
	} else {
		sys_name = emul->sysnames[ktr->ktr_code];
		(void)printf("%s", sys_name);
	}
#ifdef _LP64
#define NETBSD32_	"netbsd32_"
	if (cur_emul->flags & EMUL_FLAG_NETBSD32) {
		size_t len = strlen(NETBSD32_);
		if (strncmp(sys_name, NETBSD32_, len) == 0)
			sys_name += len;
	}
#undef NETBSD32_
#endif

	ap = (register_t *)((char *)ktr + sizeof(struct ktr_syscall));
	if (argcount) {
		c = '(';
		if (plain) {
			;

		} else if (strcmp(sys_name, "exit_group") == 0 ||
			   (strcmp(emul->name, "linux") != 0 &&
			    strcmp(emul->name, "linux32") != 0 &&
			    strcmp(sys_name, "exit") == 0)) {
			ectx_delete();

		} else if (strcmp(sys_name, "ioctl") == 0 && argcount >= 2) {
			(void)putchar('(');
			output_long((long)*ap, !(decimal || small(*ap)));
			ap++;
			argcount--;
			if ((cp = ioctlname(*ap)) != NULL)
				(void)printf(",%s", cp);
			else {
				(void)putchar(',');
				ioctldecode(*ap);
			}
			ap++;
			argcount--;
			c = ',';

		} else if ((strstr(sys_name, "sigaction") != NULL ||
		    strstr(sys_name, "sigvec") != NULL) && argcount >= 1) {
			(void)printf("(SIG%s", signame(ap[0], 1));
			ap += 1;
			argcount -= 1;
			c = ',';

		} else if ((strcmp(sys_name, "kill") == 0 ||
		    strcmp(sys_name, "killpg") == 0) && argcount >= 2) {
			putchar('(');
			output_long((long)ap[0], !(decimal || small(*ap)));
			(void)printf(", SIG%s", signame(ap[1], 1));
			ap += 2;
			argcount -= 2;
			c = ',';

		} else if (strcmp(sys_name, "ptrace") == 0 && argcount >= 1) {
			putchar('(');
			if (strcmp(emul->name, "linux") == 0 ||
			    strcmp(emul->name, "linux32") == 0) {
				if ((long)*ap >= 0 && *ap <
				    (register_t)(sizeof(linux_ptrace_ops) /
				    sizeof(linux_ptrace_ops[0])))
					(void)printf("%s",
					    linux_ptrace_ops[*ap]);
				else
					output_long((long)*ap, 1);
			} else {
				if ((long)*ap >= 0 && *ap < (register_t)
				    __arraycount(ptrace_ops))
					(void)printf("%s", ptrace_ops[*ap]);
#ifdef PT_MACHDEP_STRINGS
				else if (*ap >= PT_FIRSTMACH &&
				    *ap - PT_FIRSTMACH < (register_t)
				    __arraycount(ptrace_machdep_ops))
					(void)printf("%s", ptrace_machdep_ops[*ap - PT_FIRSTMACH]);
#endif
				else
					output_long((long)*ap, 1);
			}
			ap++;
			argcount--;
			c = ',';

		}
		while (argcount > 0) {
			putchar(c);
			output_long((long)*ap, !(decimal || small(*ap)));
			ap++;
			argcount--;
			c = ',';
		}
		(void)putchar(')');
	}
	(void)putchar('\n');
}

static void
ktrsysret(struct ktr_sysret *ktr, int len)
{
	const struct emulation *emul;
	int error = ktr->ktr_error;
	int code = ktr->ktr_code;

	if (emul_changed)  {
		/* In order to get system call name right in execve return */
		emul = prev_emul;
		emul_changed = 0;
	} else
		emul = cur_emul;

	if (numeric || ((code >= emul->nsysnames || code < 0 || plain > 1)))
		(void)printf("[%d] ", code);
	else
		(void)printf("%s ", emul->sysnames[code]);

	switch (error) {
	case 0:
		rprint(ktr->ktr_retval);
		if (len > (int)offsetof(struct ktr_sysret, ktr_retval_1) &&
		    ktr->ktr_retval_1 != 0) {
			(void)printf(", ");
			rprint(ktr->ktr_retval_1);
		}
		break;

	default:
		eprint(error);
		break;
	}
	(void)putchar('\n');
}

static void
ktrexecfd(struct ktr_execfd *ktr)
{
	static const char *dnames[] = { DTYPE_NAMES };
	if (ktr->ktr_dtype < __arraycount(dnames))
		printf("%s %d\n", dnames[ktr->ktr_dtype], ktr->ktr_fd);
	else
		printf("UNKNOWN(%u) %d\n", ktr->ktr_dtype, ktr->ktr_fd);
}

static void
rprint(register_t ret)
{

	if (!plain) {
		(void)printf("%ld", (long)ret);
		if (!small(ret))
			(void)printf("/%#lx", (long)ret);
	} else {
		if (decimal || small(ret))
			(void)printf("%ld", (long)ret);
		else
			(void)printf("%#lx", (long)ret);
	}
}

/*
 * We print the original emulation's error numerically, but we
 * translate it to netbsd to print it symbolically.
 */
static void
eprint(int e)
{
	int i = e;

	if (cur_emul->errnomap) {

		/* No remapping for ERESTART and EJUSTRETURN */
		/* Kludge for linux that has negative error numbers */
		if (cur_emul->errnomap[2] > 0 && e < 0)
			goto normal;

		for (i = 0; i < cur_emul->nerrnomap; i++)
			if (e == cur_emul->errnomap[i])
				break;

		if (i == cur_emul->nerrnomap) {
			printf("-1 unknown errno %d", e);
			return;
		}
	}

normal:
	switch (i) {
	case ERESTART:
		(void)printf("RESTART");
		break;

	case EJUSTRETURN:
		(void)printf("JUSTRETURN");
		break;

	default:
		(void)printf("-1 errno %d", e);
		if (!plain)
			(void)printf(" %s", strerror(i));
	}
}

static void
ktrnamei(char *cp, int len)
{

	(void)printf("\"%.*s\"\n", len, cp);
}

static void
ktremul(char *name, int len, int bufsize)
{

	if (len >= bufsize)
		len = bufsize - 1;

	name[len] = '\0';
	setemul(name, ktr_header.ktr_pid, 1);
	emul_changed = 1;

	(void)printf("\"%s\"\n", name);
}

static void
hexdump_buf(const void *vdp, int datalen, int word_sz)
{
	const char hex[] = "0123456789abcdef";
	char chars[16], prev[16];
	char bytes[16 * 3 + 4];
	const unsigned char *dp = vdp;
	const unsigned char *datalim = dp + datalen;
	const unsigned char *line_end;
	int off, l = 0, c;
	char *cp, *bp;
	int divmask = word_sz - 1;	/* block size in bytes */
	int gdelim = 3;			/* gap between blocks */
	int bsize = 2;			/* increment for each byte */
	int width;
	int dupl = 0;
#if _BYTE_ORDER == _LITTLE_ENDIAN
	int bswap = word_sz - 1;
#else
#define	bswap 0
#endif

	switch (word_sz) {
	case 2:
		gdelim = 2;
		break;
	case 1:
		divmask = 7;
		bsize = 3;
		gdelim = 1;
		break;
	default:
		break;
	}
	width = 16 * bsize + (16 / (divmask + 1)) * gdelim;
	if (word_sz != 1)
		width += 2;

	for (off = 0; dp < datalim; off += l) {
		memset(bytes, ' ', sizeof bytes);
		line_end = dp + 16;
		if (line_end >= datalim) {
			line_end = datalim;
			dupl |= 1;	/* need to print */
		} else {
			if (dupl == 0 || memcmp(dp, prev, sizeof chars))
				dupl |= 1;
		}

		if (!(dupl & 1)) {
			/* This is a duplicate of the line above, count 'em */
			dupl += 2;
			dp = line_end;
			continue;
		}

		if (dupl > 3) {
			/* previous line as a duplicate */
			if (dupl == 5)
				/* Only one duplicate, print line */
				printf("\t%-5.3x%.*s%.*s\n",
					off - l, width, bytes, l, chars);
			else
				printf("\t%.*s\n",
					snprintf(NULL, 0, "%3x", off), "*****");
		}

		for (l = 0, bp = bytes, cp = chars; dp < line_end; l++) {
			c = *dp++;
			prev[l] = c;
			if ((l & divmask) == 0)
				bp += gdelim;
			bp[(l ^ bswap) * bsize] = hex[c >> 4];
			bp[(l ^ bswap) * bsize + 1] = hex[c & 0xf];
			*cp++ = isgraph(c) ? c : '.';
		}

		printf("\t%-5.3x%.*s%.*s\n", off, width, bytes, l, chars);
		dupl = 2;
	}
}

static void
visdump_buf(const void *vdp, int datalen, int col)
{
	const unsigned char *dp = vdp;
	char *cp;
	int width;
	char visbuf[5];
	static int screenwidth = 0;

	if (screenwidth == 0) {
		struct winsize ws;

		if (!plain && ioctl(fileno(stderr), TIOCGWINSZ, &ws) != -1 &&
		    ws.ws_col > 8)
			screenwidth = ws.ws_col;
		else
			screenwidth = 80;
	}

	(void)printf("\"");
	col++;
	for (; datalen > 0; datalen--, dp++) {
		(void)svis(visbuf, *dp, VIS_CSTYLE,
		    datalen > 1 ? *(dp + 1) : 0, "\"\n");
		cp = visbuf;
		/*
		 * Keep track of printables and
		 * space chars (like fold(1)).
		 */
		if (col == 0) {
			(void)putchar('\t');
			col = 8;
		}
		switch (*cp) {
		case '\n':
			col = 0;
			(void)putchar('\n');
			continue;
		case '\t':
			width = 8 - (col & 07);
			break;
		default:
			width = strlen(cp);
		}
		if (col + width > (screenwidth - 2)) {
			(void)printf("\\\n\t");
			col = 8;
			if (*cp == '\t')
				width = 8;
		}
		col += width;
		do {
			(void)putchar(*cp++);
		} while (*cp);
	}
	if (col == 0)
		(void)printf("       ");
	(void)printf("\"\n");
}

static void
ktrgenio(struct ktr_genio *ktr, int len)
{
	int datalen = len - sizeof (struct ktr_genio);
	char *dp = (char *)ktr + sizeof (struct ktr_genio);

	if (ktr->ktr_fd != -1)
		printf("fd %d ", ktr->ktr_fd);
	printf("%s %d bytes\n", 
	    ktr->ktr_rw == UIO_READ ? "read" : "wrote", datalen);
	if (maxdata == 0)
		return;
	if (maxdata > 0 && datalen > maxdata)
		datalen = maxdata;
	if (word_size) {
		hexdump_buf(dp, datalen, word_size);
		return;
	}
	(void)printf("       ");
	visdump_buf(dp, datalen, 7);
}

static void
ktrpsig(void *v, int len)
{
	int signo, first;
	struct {
		struct ktr_psig ps;
		siginfo_t si;
	} *psig = v;
	siginfo_t *si = &psig->si;
	const char *code;

	(void)printf("SIG%s ", signame(psig->ps.signo, 0));
	if (psig->ps.action == SIG_DFL)
		(void)printf("SIG_DFL");
	else {
		(void)printf("caught handler=%p mask=(", psig->ps.action);
		first = 1;
		for (signo = 1; signo < NSIG; signo++) {
			if (sigismember(&psig->ps.mask, signo)) {
				if (first)
					first = 0;
				else
					(void)printf(",");
				(void)printf("%d", signo);
			}
		}
		(void)printf(")");
	}
	switch (len) {
	case sizeof(struct ktr_psig):
		if (psig->ps.code)
			printf(" code=0x%x", psig->ps.code);
		printf(psig->ps.action == SIG_DFL ? "\n" : ")\n");
		return;
	case sizeof(*psig):
		if (si->si_code == 0) {
			printf(": code=SI_USER sent by pid=%d, uid=%d)\n",
			    si->si_pid, si->si_uid);
			return;
		}

		if (si->si_code < 0) {
			switch (si->si_code) {
			case SI_TIMER:
			case SI_QUEUE:
				printf(": code=%s sent by pid=%d, uid=%d with "
				    "sigval %p)\n", si->si_code == SI_TIMER ?
				    "SI_TIMER" : "SI_QUEUE", si->si_pid,
				    si->si_uid, si->si_value.sival_ptr);
				return;
			case SI_ASYNCIO:
			case SI_MESGQ:
				printf(": code=%s with sigval %p)\n",
				    si->si_code == SI_ASYNCIO ?
				    "SI_ASYNCIO" : "SI_MESGQ",
				    si->si_value.sival_ptr);
				return;
			case SI_LWP:
				printf(": code=SI_LWP sent by pid=%d, "
				    "uid=%d)\n", si->si_pid, si->si_uid);
				return;
			default:
				code = NULL;
				break;
			}
			if (code)
				printf(": code=%s unimplemented)\n", code);
			else
				printf(": code=%d unimplemented)\n",
				    si->si_code);
			return;
		}

		if (si->si_code == SI_NOINFO) {
			printf(": code=SI_NOINFO\n");
			return;
		}

		code = siginfocodename(si->si_signo, si->si_code);
		switch (si->si_signo) {
		case SIGCHLD:
			printf(": code=%s child pid=%d, uid=%d, "
			    " status=%u, utime=%lu, stime=%lu)\n",
			    code, si->si_pid,
			    si->si_uid, si->si_status,
			    (unsigned long) si->si_utime,
			    (unsigned long) si->si_stime);
			return;
		case SIGILL:
		case SIGFPE:
		case SIGSEGV:
		case SIGBUS:
		case SIGTRAP:
			printf(": code=%s, addr=%p, trap=%d)\n",
			    code, si->si_addr, si->si_trap);
			return;
		case SIGIO:
			printf(": code=%s, fd=%d, band=%lx)\n",
			    code, si->si_fd, si->si_band);
			return;
		default:
			printf(": code=%s, errno=%d)\n",
			    code, si->si_errno);
			return;
		}
		/*NOTREACHED*/
	default:
		warnx("Unhandled size %d for ktrpsig\n", len);
		break;
	}
}

static void
ktrcsw(struct ktr_csw *cs)
{

	(void)printf("%s %s\n", cs->out ? "stop" : "resume",
	    cs->user ? "user" : "kernel");
}

static void
ktruser_msghdr(const char *name, const void *buf, size_t len)
{
	struct msghdr m;

	if (len != sizeof(m))
		warnx("%.*s: len %zu != %zu", KTR_USER_MAXIDLEN, name, len,
		    sizeof(m));
	memcpy(&m, buf, len);
	printf("%.*s: [name=%p, namelen=%zu, iov=%p, iovlen=%zu, control=%p, "
	    "controllen=%zu, flags=%x]\n", KTR_USER_MAXIDLEN, name,
	    m.msg_name, (size_t)m.msg_namelen, m.msg_iov, (size_t)m.msg_iovlen,
	    m.msg_control, (size_t)m.msg_controllen, m.msg_flags);
}

static void
ktruser_soname(const char *name, const void *buf, size_t len)
{
	char fmt[512];
	sockaddr_snprintf(fmt, sizeof(fmt), "%a", buf);
	printf("%.*s: [%s]\n", KTR_USER_MAXIDLEN, name, fmt);
}

static void
ktruser_control(const char *name, const void *buf, size_t len)
{
	struct cmsghdr m;

	if (len < sizeof(m))
		warnx("%.*s: len %zu < %zu", KTR_USER_MAXIDLEN, name, len,
		    sizeof(m));
	memcpy(&m, buf, sizeof(m));
	printf("%.*s: [len=%zu, level=%d, type=%d]\n", KTR_USER_MAXIDLEN, name,
	    (size_t)m.cmsg_len, m.cmsg_level, m.cmsg_type);
}

static void
ktruser_misc(const char *name, const void *buf, size_t len)
{
	size_t i;
	const char *dta = buf;

	printf("%.*s: %zu, ", KTR_USER_MAXIDLEN, name, len);
	for (i = 0; i < len; i++)
		printf("%02x", (unsigned int) dta[i]);
	printf("\n");
}

static struct {
	const char *name;
	void (*func)(const char *, const void *, size_t);
} nv[] = {
	{ "msghdr", ktruser_msghdr },
	{ "mbsoname", ktruser_soname },
	{ "mbcontrol", ktruser_control },
	{ NULL,	ktruser_misc },
};

static void
ktruser(struct ktr_user *usr, int len)
{
	unsigned char *dta;

	len -= sizeof(struct ktr_user);
	dta = (unsigned char *)(usr + 1);
	if (word_size) {
		printf("%.*s:", KTR_USER_MAXIDLEN, usr->ktr_id);
		printf("\n");
		hexdump_buf(dta, len, word_size);
		return;
	}
	for (size_t j = 0; j < __arraycount(nv); j++)
		if (nv[j].name == NULL ||
		    strncmp(nv[j].name, usr->ktr_id, KTR_USER_MAXIDLEN) == 0) {
			(*nv[j].func)(usr->ktr_id, dta, len);
			break;
		}
}

static void
ktrmib(int *namep, int len)
{
	size_t i;

	for (i = 0; i < (len / sizeof(*namep)); i++)
		printf("%s%d", (i == 0) ? "" : ".", namep[i]);
	printf("\n");
}

static const char *
signame(long sig, int xlat)
{
	static char buf[64];

	if (sig == 0)
		return " 0";
	else if (sig < 0 || sig >= NSIG) {
		(void)snprintf(buf, sizeof(buf), "*unknown %ld*", sig);
		return buf;
	} else
		return sys_signame[(xlat && cur_emul->signalmap != NULL) ?
		    cur_emul->signalmap[sig] : sig];
}

static void
usage(void)
{
	if (strcmp(getprogname(), "ioctlname") == 0) {
		(void)fprintf(stderr, "Usage: %s [-e emulation] <ioctl> ...\n",
		    getprogname());
	} else {
		(void)fprintf(stderr, "Usage: %s [-dlNnRT] [-e emulation] "
		   "[-f file] [-m maxdata] [-p pid]\n             [-t trstr] "
		   "[-x | -X size] [file]\n", getprogname());
	}
	exit(1);
}

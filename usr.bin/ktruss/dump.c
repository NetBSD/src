/*	$NetBSD: dump.c,v 1.29.4.1 2009/06/21 01:08:21 snj Exp $	*/

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
#endif
__RCSID("$NetBSD: dump.c,v 1.29.4.1 2009/06/21 01:08:21 snj Exp $");
#endif /* not lint */

#include <sys/param.h>
#define	_KERNEL
#include <sys/errno.h>
#undef _KERNEL
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/ktrace.h>
#include <sys/ptrace.h>
#include <sys/queue.h>

#include <err.h>
#include <signal.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>

#include "ktrace.h"
#include "misc.h"
#include "setemul.h"

int timestamp, decimal, fancy = 1, tail, maxdata;

int width;			/* Keep track of current columns. */

#include <sys/syscall.h>

static const char *const ptrace_ops[] = {
	"PT_TRACE_ME",	"PT_READ_I",	"PT_READ_D",	"PT_READ_U",
	"PT_WRITE_I",	"PT_WRITE_D",	"PT_WRITE_U",	"PT_CONTINUE",
	"PT_KILL",	"PT_ATTACH",	"PT_DETACH",
};

struct ktr_entry {
	TAILQ_ENTRY(ktr_entry) kte_list;
	struct ktr_header kte_kth;
};

TAILQ_HEAD(kteq, ktr_entry) ktependq = TAILQ_HEAD_INITIALIZER(ktependq);

void	argprint(const char *, register_t **, int *);
void	dumpheader(struct ktr_header *);
int	dumprecord(int, FILE *);
void	flushpendq(struct ktr_entry *);
int	fread_tail(void *, int, int, FILE *);
void	genioprint(struct ktr_header *);
struct ktr_entry *
	getpendq(struct ktr_header *, int, struct kteq *);
struct ktr_entry *
	getrecord(FILE *);
void	indent(int);
void	ioctldecode(u_long);
void	ktrcsw(struct ktr_entry *);
void	ktremul(struct ktr_entry *);
void	ktrgenio(struct ktr_entry *);
void	ktrnamei(struct ktr_entry *);
void	ktrpsig(struct ktr_entry *);
void	ktrsyscall(struct ktr_entry *);
void	ktrsysret(struct ktr_entry *);
void	nameiargprint(const char *, struct ktr_header *, register_t **, int *);
void	nameiprint(struct ktr_header *);
void	newline(void);
void	putpendq(struct ktr_entry *);
void	syscallnameprint(int);
void	syscallprint(struct ktr_header *);
void	sysretprint(struct ktr_header *);
int	wprintf(const char *, ...);
void	*xrealloc(void *, size_t *, size_t);

int
wprintf(const char *fmt, ...)
{
	va_list ap;
	int w;

	va_start(ap, fmt);
	w = vprintf(fmt, ap);
	if (w == -1)
		warn("vprintf");
	else
		width += w;
	va_end(ap);
	return (w);
}

void
newline(void)
{

	if (width > 0) {
		printf("\n");
		width = 0;
	}
}

void
indent(int col)
{

	while (width < col)
		if (wprintf(" ") < 0)
			break;
}

void *
xrealloc(void *p, size_t *siz, size_t req)
{

	if (*siz < req) {
		if (*siz == 0)
			*siz = 1;
		while (*siz < req)
			*siz <<= 1;
		p = realloc(p, *siz);
		if (p == NULL)
			err(EXIT_FAILURE, "realloc: %lu bytes",
			    (u_long)*siz);
	}
	return (p);
}

struct ktr_entry *
getrecord(FILE *fp)
{
	struct ktr_entry *kte;
	struct ktr_header *kth;
	char *cp;
	size_t siz, len;

	siz = 0;
	kte = xrealloc(NULL, &siz, sizeof(struct ktr_entry));
	kth = &kte->kte_kth;
	if (fread_tail(kth, sizeof(struct ktr_header), 1, fp) == 0) {
		free(kte);
		return (NULL);
	}

	if (kth->ktr_len < 0)
		errx(EXIT_FAILURE, "bogus length 0x%x", kth->ktr_len);
	len = kth->ktr_len;
	if (len > 0) {
		/* + 1 to ensure room for NUL terminate */
		kte = xrealloc(kte, &siz, sizeof(struct ktr_entry) + len + 1);
		if (fread_tail(cp = (char *)(&kte->kte_kth + 1),
		    len, 1, fp) == 0)
			errx(EXIT_FAILURE, "data too short");
		cp[len] = 0;
	}

	return (kte);
}

#define	KTE_TYPE(kte)		((kte)->kte_kth.ktr_type)
#define	KTE_PID(kte)		((kte)->kte_kth.ktr_pid)
#define	KTE_LID(kte)		((kte)->kte_kth.ktr_lid)
#define	KTE_MATCH(kte, type, pid, lid)				\
	(KTE_TYPE(kte) == (type) && KTE_PID(kte) == (pid) &&	\
	KTE_LID(kte) == (lid))

void
putpendq(struct ktr_entry *kte)
{

	TAILQ_INSERT_TAIL(&ktependq, kte, kte_list);
}

void
flushpendq(struct ktr_entry *us)
{
	struct ktr_entry *kte, *kte_next;
	int pid = KTE_PID(us);

	for (kte = TAILQ_FIRST(&ktependq); kte != NULL; kte = kte_next) {
		kte_next = TAILQ_NEXT(kte, kte_list);
		if (KTE_PID(kte) == pid) {
			TAILQ_REMOVE(&ktependq, kte, kte_list);
			free(kte);
		}
	}
}

struct ktr_entry *
getpendq(struct ktr_header *us, int type, struct kteq *kteq)
{
	struct ktr_entry *kte, *kte_next;
	int pid = us->ktr_pid, lid = us->ktr_lid;

	if (kteq != NULL)
		TAILQ_INIT(kteq);
	for (kte = TAILQ_FIRST(&ktependq); kte != NULL; kte = kte_next) {
		kte_next = TAILQ_NEXT(kte, kte_list);
		if (KTE_MATCH(kte, type, pid, lid)) {
			TAILQ_REMOVE(&ktependq, kte, kte_list);
			if (kteq != NULL)
				TAILQ_INSERT_TAIL(kteq, kte, kte_list);
			else
				break;
		}
	}

	return (kteq ? TAILQ_FIRST(kteq) : kte);
}

int
dumprecord(int trpoints, FILE *fp)
{
	struct ktr_entry *kte;
	struct ktr_header *kth;

	kte = getrecord(fp);
	if (kte == NULL)
		return (0);

	kth = &kte->kte_kth;
	if ((trpoints & (1 << kth->ktr_type)) == 0) {
		free(kte);
		goto out;
	}

	/* Update context to match currently processed record. */
	ectx_sanify(kth->ktr_pid);

	switch (kth->ktr_type) {
	case KTR_SYSCALL:
		ktrsyscall(kte);
		break;
	case KTR_SYSRET:
		ktrsysret(kte);
		break;
	case KTR_NAMEI:
		putpendq(kte);
		break;
	case KTR_GENIO:
		putpendq(kte);
		break;
	case KTR_PSIG:
		ktrpsig(kte);
		break;
	case KTR_CSW:
		ktrcsw(kte);
		break;
	case KTR_EMUL:
		ktremul(kte);
		break;
	default:
		/*
		 * XXX: Other types added recently.
		 */
		free(kte);
		break;
	}
	newline();

out:
	return (1);
}

void
dumpfile(const char *file, int fd, int trpoints)
{
	FILE *fp;

	if (file == NULL || *file == 0) {
		if ((fp = fdopen(fd, "r")) == NULL)
			err(EXIT_FAILURE, "fdopen(%d)", fd);
	} else if (strcmp(file, "-") == 0)
		fp = stdin;
	else if ((fp = fopen(file, "r")) == NULL)
		err(EXIT_FAILURE, "fopen: %s", file);

	for (width = 0; dumprecord(trpoints, fp) != 0;)
		if (tail)
			(void)fflush(stdout);

	newline();

	/*
	 * XXX: Dump pending KTR_SYSCALL if any?
	 */
}

int
fread_tail(void *buf, int size, int num, FILE *fp)
{
	int i;

	while ((i = fread(buf, size, num, fp)) == 0 && tail) {
		(void)sleep(1);
		clearerr(fp);
	}
	return (i);
}

void
dumpheader(struct ktr_header *kth)
{
	union timeholder {
		struct timeval tv;
		struct timespec ts;
	};
	static union timeholder prevtime;
	union timeholder temp;

	wprintf("%6d ", kth->ktr_pid);
	if (kth->ktr_version > KTRFACv0)
		wprintf("%6d ", kth->ktr_lid);
	wprintf("%-8.*s ", MAXCOMLEN, kth->ktr_comm);
	if (timestamp) {
		if (timestamp == 2) {
			if (kth->ktr_version == KTRFACv0) {
				if (prevtime.tv.tv_sec == 0)
					temp.tv.tv_sec = temp.tv.tv_usec = 0;
				else
					timersub(&kth->ktr_tv,
					    &prevtime.tv, &temp.tv);
				prevtime.tv = kth->ktr_tv;
			} else {
				if (prevtime.ts.tv_sec == 0)
					temp.ts.tv_sec = temp.ts.tv_nsec = 0;
				else
					timespecsub(&kth->ktr_time,
					    &prevtime.ts, &temp.ts);
				prevtime.ts = kth->ktr_time;
			}
		} else {
			if (kth->ktr_version == KTRFACv0)
				temp.tv = kth->ktr_tv;
			else
				temp.ts = kth->ktr_time;
		}
		if (kth->ktr_version == KTRFACv0)
			wprintf("%ld.%06ld ",
			    (long)temp.tv.tv_sec, (long)temp.tv.tv_usec);
		else
			wprintf("%ld.%09ld ",
			    (long)temp.ts.tv_sec, (long)temp.ts.tv_nsec);
	}
}

void
ioctldecode(u_long cmd)
{
	char dirbuf[4], *dir = dirbuf;

	if (cmd & IOC_OUT)
		*dir++ = 'W';
	if (cmd & IOC_IN)
		*dir++ = 'R';
	*dir = '\0';

	wprintf(decimal ? ", _IO%s('%c',%ld" : ", _IO%s('%c',%#lx",
	    dirbuf, (int) ((cmd >> 8) & 0xff), cmd & 0xff);
	if ((cmd & IOC_VOID) == 0)
		wprintf(decimal ? ",%ld)" : ",%#lx)",
		    (cmd >> 16) & 0xff);
	else
		wprintf(")");
}

void
nameiargprint(const char *prefix, struct ktr_header *kth,
    register_t **ap, int *argsize)
{
	struct ktr_entry *kte;

	if (*argsize == 0)
		errx(EXIT_FAILURE, "argument expected");
	/*
	 * XXX: binary emulation mode.
	 */
	kte = getpendq(kth, KTR_NAMEI, NULL);
	if (kte == NULL)
		argprint(prefix, ap, argsize);
	else {
		wprintf("%s", prefix);
		nameiprint(&kte->kte_kth);
		free(kte);
		(*ap)++;
		*argsize -= sizeof(register_t);
	}
}

void
syscallnameprint(int code)
{

	if (code >= cur_emul->nsysnames || code < 0)
		wprintf("[%d]", code);
	else
		wprintf("%s", cur_emul->sysnames[code]);
}

void
argprint(const char *prefix, register_t **ap, int *argsize)
{

	if (decimal)
		wprintf("%s%ld", prefix, (long)**ap);
	else
		wprintf("%s%#lx", prefix, (long)**ap);
	(*ap)++;
	*argsize -= sizeof(register_t);
}

void
syscallprint(struct ktr_header *kth)
{
	struct ktr_syscall *ktr = (struct ktr_syscall *)(kth + 1);
	register_t *ap;
	const char *s;
	int argsize;

	syscallnameprint(ktr->ktr_code);

	/*
	 * Arguments processing.
	 */
	argsize = ktr->ktr_argsize;
	if (argsize == 0) {
		wprintf("(");
		goto noargument;
	}

	ap = (register_t *)(ktr + 1);
	if (!fancy)
		goto print_first;

	switch (ktr->ktr_code) {
	/*
	 * All these have a path as the first param.
	 * The order is same as syscalls.master.
	 */
	case SYS_open:
	case SYS_link:
	case SYS_unlink:
	case SYS_chdir:
	case SYS_mknod:
	case SYS_chmod:
	case SYS_chown:
	case SYS_unmount:
	case SYS_access:
	case SYS_chflags:
	case SYS_acct:
	case SYS_revoke:
	case SYS_symlink:
	case SYS_readlink:
	case SYS_execve:
	case SYS_chroot:
	case SYS_rename:
	case SYS_mkfifo:
	case SYS_mkdir:
	case SYS_rmdir:
	case SYS_utimes:
	case SYS_quotactl:
	case SYS_statvfs1:
	case SYS_compat_30_getfh:
	case SYS_pathconf:
	case SYS_truncate:
	case SYS_undelete:
	case SYS___posix_rename:
	case SYS_lchmod:
	case SYS_lchown:
	case SYS_lutimes:
	case SYS___stat30:
	case SYS___lstat30:
	case SYS___posix_chown:
	case SYS___posix_lchown:
	case SYS_lchflags:
	case SYS___getfh30:
		nameiargprint("(", kth, &ap, &argsize);

		/*
		 * 2nd argument is also pathname.
		 */
		switch (ktr->ktr_code) {
		case SYS_link:
		case SYS_rename:
		case SYS___posix_rename:
			nameiargprint(", ", kth, &ap, &argsize);
			break;
		}
		break;

	case SYS_compat_16___sigaction14 :
		wprintf("(%s", signals[(int)*ap].name);
		ap++;
		argsize -= sizeof(register_t);
		break;

	case SYS_ioctl :
		argprint("(", &ap, &argsize);
		if ((s = ioctlname(*ap)) != NULL)
			wprintf(", %s", s);
		else
			ioctldecode(*ap);
		ap++;
		argsize -= sizeof(register_t);
		break;

	case SYS_ptrace :
		if ((long)*ap >= 0 &&
		    *ap < sizeof(ptrace_ops) / sizeof(ptrace_ops[0]))
			wprintf("(%s", ptrace_ops[*ap]);
		else
			wprintf("(%ld", (long)*ap);
		ap++;
		argsize -= sizeof(register_t);
		break;

	default:
print_first:
		argprint("(", &ap, &argsize);
		break;
	}

	/* Print rest of argument. */
	while (argsize > 0)
		argprint(", ", &ap, &argsize);

noargument:
	wprintf(")");
}

void
ktrsyscall(struct ktr_entry *kte)
{
	struct ktr_header *kth = &kte->kte_kth;
	struct ktr_syscall *ktr = (struct ktr_syscall *)(kth + 1);

	switch (ktr->ktr_code) {
	case SYS_exit:
		dumpheader(kth);
		syscallprint(kth);
		break;
	default:
		putpendq(kte);
		return;
	}

	free(kte);
}

void
sysretprint(struct ktr_header *kth)
{
	struct ktr_sysret *ktr = (struct ktr_sysret *)(kth + 1);
	register_t ret = ktr->ktr_retval;
	int error = ktr->ktr_error;

	indent(50);
	if (error == EJUSTRETURN)
		wprintf(" JUSTRETURN");
	else if (error == ERESTART)
		wprintf(" RESTART");
	else if (error) {
		wprintf(" Err#%d", error);
		if (error < MAXERRNOS && error >= -2)
			wprintf(" %s", errnos[error].name);
	} else
		switch (ktr->ktr_code) {
		case SYS_mmap:
			wprintf(" = %p", (long)ret);
			break;
		default:
			wprintf(" = %ld", (long)ret);
			if (kth->ktr_len > offsetof(struct ktr_sysret,
			    ktr_retval_1) && ktr->ktr_retval_1 != 0)
				wprintf(", %ld", (long)ktr->ktr_retval_1);
			break;
		}
}

void
ktrsysret(struct ktr_entry *kte)
{
	struct ktr_header *kth = &kte->kte_kth;
	struct ktr_sysret *ktr = (struct ktr_sysret *)(kth + 1);
	struct ktr_entry *genio;
	struct ktr_entry *syscall_ent;

	dumpheader(kth);

	/* Print syscall name and arguments. */
	syscall_ent = getpendq(kth, KTR_SYSCALL, NULL);
	if (syscall_ent == NULL) {
		/*
		 * Possibilly a child of fork/vfork, or tracing of
		 * process started during system call.
		 */
		syscallnameprint(ktr->ktr_code);
	} else {
		syscallprint(&syscall_ent->kte_kth);
		free(syscall_ent);
	}

	/* Print return value and an error if any. */
	sysretprint(kth);

	genio = getpendq(kth, KTR_GENIO, NULL);
	if (genio != NULL) {
		genioprint(&genio->kte_kth);
		free(genio);
	}

	flushpendq(kte);
	free(kte);
}

void
nameiprint(struct ktr_header *kth)
{

	wprintf("\"%.*s\"", kth->ktr_len, (char *)(kth + 1));
}

#ifdef notused
void
ktrnamei(struct ktr_entry *kte)
{
	struct ktr_header *kth = &kte->kte_kth;

	dumpheader(kth);
	wprintf("namei(");
	nameiprint(kth);
	wprintf(")");

	free(kte);
}
#endif

void
ktremul(struct ktr_entry *kte)
{
	struct ktr_header *kth = &kte->kte_kth;
	char *emul = (char *)(kth + 1);

	dumpheader(kth);
	wprintf("emul(%s)", emul);
	setemul(emul, kth->ktr_pid, 1);

	free(kte);
}

void
genioprint(struct ktr_header *kth)
{
	struct ktr_genio *ktr = (struct ktr_genio *)(kth + 1);
	static int screenwidth = 0;
	int datalen = kth->ktr_len - sizeof(struct ktr_genio);
	/*
	 * Need to be unsigned type so that positive value is passed
	 * to vis(), which will call isgraph().
	 */
	unsigned char *dp = (unsigned char *)(ktr + 1);
	int w;
	char visbuf[5];

	if (screenwidth == 0) {
		struct winsize ws;

		if (fancy && ioctl(fileno(stderr), TIOCGWINSZ, &ws) != -1 &&
		    ws.ws_col > 8)
			screenwidth = ws.ws_col;
		else
			screenwidth = 80;
	}

	if (maxdata && datalen > maxdata)
		datalen = maxdata;
	newline();
	wprintf("       \"");
	for (; datalen > 0; datalen--, dp++) {
		(void) vis(visbuf, *dp, VIS_NL|VIS_TAB|VIS_CSTYLE,
		    /* We put NUL at the end of buffer when reading */
		    *(dp + 1));
		visbuf[4] = '\0';
		w = strlen(visbuf);
		if (width + w + 2 >= screenwidth)
			break;
		wprintf("%s", visbuf);
		if (width + 2 >= screenwidth)
			break;
	}
	wprintf("\"");
}

#ifdef notused
void
ktrgenio(struct ktr_entry *kte)
{
	struct ktr_header *kth = &kte->kte_kth;
	struct ktr_genio *ktr = (struct ktr_genio *)(kth + 1);

	dumpheader(kth);
	wprintf("genio fd %d %s",
	    ktr->ktr_fd, ktr->ktr_rw ? "write" : "read");
	genioprint(kth);

	free(kte);
}
#endif

void
ktrpsig(struct ktr_entry *kte)
{
	struct ktr_header *kth = &kte->kte_kth;
	struct ktr_psig *psig = (struct ktr_psig *)(kth + 1);

	dumpheader(kth);
	wprintf("SIG%s ", sys_signame[psig->signo]);
	if (psig->action == SIG_DFL)
		wprintf("SIG_DFL");
	else {
		wprintf("caught handler=0x%lx mask=0x%lx code=0x%x",
		    (u_long)psig->action, (unsigned long)psig->mask.__bits[0],
		    psig->code);
	}

	free(kte);
}

void
ktrcsw(struct ktr_entry *kte)
{
	struct ktr_header *kth = &kte->kte_kth;
	struct ktr_csw *cs = (struct ktr_csw *)(kth + 1);

	dumpheader(kth);
	wprintf("%s %s", cs->out ? "stop" : "resume",
	    cs->user ? "user" : "kernel");

	free(kte);
}

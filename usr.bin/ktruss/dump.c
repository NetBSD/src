/*	$NetBSD: dump.c,v 1.14 2003/03/16 09:59:09 jdolecek Exp $	*/

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
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
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
__COPYRIGHT("@(#) Copyright (c) 1988, 1993\n\
	The Regents of the University of California.  All rights reserved.\n");
#endif /* not lint */

#ifndef lint
#if 0
static char sccsid[] = "@(#)kdump.c	8.4 (Berkeley) 4/28/95";
#endif
__RCSID("$NetBSD: dump.c,v 1.14 2003/03/16 09:59:09 jdolecek Exp $");
#endif /* not lint */

#include <sys/param.h>
#define	_KERNEL
#include <sys/errno.h>
#undef _KERNEL
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/ktrace.h>
#include <sys/ioctl.h>
#include <sys/ptrace.h>
#define _KERNEL
#include <sys/errno.h>
#undef _KERNEL

#include <err.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vis.h>

#include "ktrace.h"
#include "misc.h"
#include "setemul.h"

int timestamp, decimal, fancy = 1, tail, maxdata;

#define eqs(s1, s2)	(strcmp((s1), (s2)) == 0)

#include <sys/syscall.h>

static const char * const ptrace_ops[] = {
	"PT_TRACE_ME",	"PT_READ_I",	"PT_READ_D",	"PT_READ_U",
	"PT_WRITE_I",	"PT_WRITE_D",	"PT_WRITE_U",	"PT_CONTINUE",
	"PT_KILL",	"PT_ATTACH",	"PT_DETACH",
};


void	dumprecord __P((struct ktr_header *, int, int *, void **, FILE *));
void	dumpheader __P((struct ktr_header *, char *, int, int *));
int	fread_tail __P((char *, int, int, FILE *));
void	ioctldecode __P((u_long));
int	ktrsyscall __P((struct ktr_syscall *, int, char *, int, int *));
void	ktrsysret __P((struct ktr_sysret *, char *, int, int *));
void	ktrnamei __P((char *, int, char *, int, int *));
void	ktremul __P((struct ktr_header *, char *, int, char *, int, int *));
void	ktrgenio __P((struct ktr_genio *, int, char *, int, int *));
void	ktrpsig __P((struct ktr_psig *));
void	ktrcsw __P((struct ktr_csw *));

#define	KTR_BUFSZ	512
#define	BLEFT	(bufsz - (bp - buff))


void
dumprecord(ktr, trpoints, sizep, mp, fp)
	register struct ktr_header *ktr;
	int trpoints;
	int *sizep;
	void **mp;
	FILE *fp;
{
	static void *mcopy = NULL;
	static int linelen = 0, iolinelen = 0;
	char buff[KTR_BUFSZ], iobuff[KTR_BUFSZ], *bp;
	int ktrlen, *lenp;
	void *m;

	if (ktr->ktr_type == KTR_GENIO || ktr->ktr_type == KTR_EMUL) {
		bp = iobuff;
		lenp = &iolinelen;
	} else {
		bp = buff;
		lenp = &linelen;
	}
	if (!mcopy && (trpoints & (1<<ktr->ktr_type)))
		dumpheader(ktr, bp, KTR_BUFSZ, lenp);

	if ((ktrlen = ktr->ktr_len) < 0)
		errx(1, "bogus length 0x%x", ktrlen);
	m = *mp;
	if (ktrlen >= *sizep) {
		while(ktrlen > *sizep) *sizep *= 2;
		*mp = m = (void *)realloc(m, *sizep);
		if (m == NULL)
			errx(1, "realloc: %s", strerror(ENOMEM));
	}
	if (ktrlen && fread_tail(m, ktrlen, 1, fp) == 0)
		errx(1, "data too short");
	if ((trpoints & (1<<ktr->ktr_type)) == 0)
		return;

	/* update context to match currently processed record */
	ectx_sanify(ktr->ktr_pid);

	switch (ktr->ktr_type)
	{
	case KTR_SYSCALL:
		if (ktrsyscall((struct ktr_syscall *)m, 0, bp, KTR_BUFSZ,
			       lenp) == 0) {
			mcopy = (void *)malloc(ktrlen + 1);
			bcopy(m, mcopy, ktrlen);
			return;
		}
		break;
	case KTR_SYSRET:
		ktrsysret((struct ktr_sysret *)m, bp, KTR_BUFSZ, lenp);
		if (*iobuff || iolinelen) {
			fputs(iobuff, stdout);
			*iobuff = '\0';
			iolinelen = 0;
		}
		break;
	case KTR_NAMEI:
		ktrnamei(m, ktrlen, bp, sizeof(buff), lenp);
		if (mcopy) {
			(void) ktrsyscall((struct ktr_syscall *)mcopy, 1, bp,
					  KTR_BUFSZ, lenp);
			free(mcopy);
			mcopy = NULL;
		}
		break;
	case KTR_GENIO:
		ktrgenio((struct ktr_genio *)m, ktrlen, bp, KTR_BUFSZ, lenp);
		break;
	case KTR_PSIG:
		ktrpsig((struct ktr_psig *)m);
		break;
	case KTR_CSW:
		ktrcsw((struct ktr_csw *)m);
		break;
	case KTR_EMUL:
		ktremul(ktr, m, ktrlen, bp, sizeof(buff), lenp);
		break;
	}

	if (mcopy) {
		free(mcopy);
		mcopy = NULL;
	}
}

void
dumpfile(file, fd, trpoints)
	const char *file;
	int fd;
	int trpoints;
{
	struct ktr_header ktr_header;
	void *m;
	FILE *fp;
	int size;

	m = (void *)malloc(size = 1024);
	if (m == NULL)
		errx(1, "malloc: %s", strerror(ENOMEM));
	if (!file || !*file) {
		if (!(fp = fdopen(fd, "r")))
			err(1, "fdopen(%d)", fd);
	} else if (!strcmp(file, "-"))
		fp = stdin;
	else if (!(fp = fopen(file, "r")))
		err(1, "%s", file);

	while (fread_tail((char *)&ktr_header,sizeof(struct ktr_header),1,fp)) {
		dumprecord(&ktr_header, trpoints, &size, &m, fp);
		if (tail)
			(void)fflush(stdout);
	}
}


int
fread_tail(buf, size, num, fp)
	char *buf;
	int num, size;
	FILE *fp;
{
	int i;

	while ((i = fread(buf, size, num, fp)) == 0 && tail) {
		(void)sleep(1);
		clearerr(fp);
	}
	return (i);
}

void
dumpheader(kth, buff, buffsz, lenp)
	struct ktr_header *kth;
	char *buff;
	int buffsz, *lenp;
{
	static struct timeval prevtime;
	char *bp = buff + *lenp;
	struct timeval temp;

	if (kth->ktr_type == KTR_SYSRET || kth->ktr_type == KTR_GENIO)
			return;
	*lenp = 0;
	(void)snprintf(bp, buffsz - *lenp, "%6d %-8.*s ",
		kth->ktr_pid, MAXCOMLEN, kth->ktr_comm);
	*lenp += strlen(bp);
	bp = buff + *lenp;

	if (timestamp) {
		if (timestamp == 2) {
			timersub(&kth->ktr_time, &prevtime, &temp);
			prevtime = kth->ktr_time;
		} else
			temp = kth->ktr_time;
		(void)snprintf(bp, buffsz - *lenp, "%ld.%06ld ",
		    (long int)temp.tv_sec,
		    (long int)temp.tv_usec);
		*lenp += strlen(bp);
	}
}

void
ioctldecode(cmd)
	u_long cmd;
{
	char dirbuf[4], *dir = dirbuf;

	if (cmd & IOC_OUT)
		*dir++ = 'W';
	if (cmd & IOC_IN)
		*dir++ = 'R';
	*dir = '\0';

	printf(decimal ? ",_IO%s('%c',%ld" : ",_IO%s('%c',%#lx",
	    dirbuf, (int) ((cmd >> 8) & 0xff), cmd & 0xff);
	if ((cmd & IOC_VOID) == 0)
		printf(decimal ? ",%ld)" : ",%#lx)", (cmd >> 16) & 0xff);
	else
		printf(")");
}

int
ktrsyscall(ktr, nohdr, buff, bufsz, lenp)
	register struct ktr_syscall *ktr;
	int nohdr, bufsz, *lenp;
	char *buff;
{
	register int argsize = ktr->ktr_argsize;
	register register_t *ap;
	char *bp = buff;
	int eol = 1;

	if (*lenp < bufsz) {
		bp += *lenp;
		bzero(bp, BLEFT);
	}
	if (!nohdr) {
		if (ktr->ktr_code >= current->nsysnames || ktr->ktr_code < 0)
			(void)snprintf(bp, BLEFT, "[%d]", ktr->ktr_code);
		else
			(void)snprintf(bp, BLEFT,
				       "%s", current->sysnames[ktr->ktr_code]);
		bp += strlen(bp);
	}
	ap = (register_t *)((char *)ktr + sizeof(struct ktr_syscall));
	if (argsize) {
		char *s = "(";
		if (fancy && !nohdr) {
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
			case SYS_statfs:
			case SYS_getfh:
			case SYS_pathconf:
			case SYS_truncate:
			case SYS_undelete:
			case SYS___posix_rename:
			case SYS_lchmod:
			case SYS_lchown:
			case SYS_lutimes:
			case SYS___stat13:
			case SYS___lstat13:
			case SYS___posix_chown:
			case SYS___posix_lchown:
			case SYS_lchflags:
				if (BLEFT > 1)
					*bp++ = '(';
				eol = 0;
				break;
			case SYS___sigaction14 :
				(void)snprintf(bp, BLEFT, "(%s",
					       signals[(int)*ap].name);
				s = ", ";
				argsize -= sizeof(register_t);
				ap++;
				break;
			case SYS_ioctl :
				if (decimal)
					(void)snprintf(bp, BLEFT, "(%ld",
						       (long)*ap);
				else
					(void)snprintf(bp, BLEFT, "(%#lx",
						       (long)*ap);
				bp += strlen(bp);
				ap++;
				argsize -= sizeof(register_t);
				if ((s = ioctlname(*ap)) != NULL)
					(void)snprintf(bp, BLEFT, ", %s", s);
				else
					ioctldecode(*ap);
				s = ", ";
				ap++;
				argsize -= sizeof(register_t);
				break;
			case SYS_ptrace :
				if (*ap >= 0 && *ap <=
				    sizeof(ptrace_ops) / sizeof(ptrace_ops[0]))
					(void)snprintf(bp, BLEFT, "(%s",
						       ptrace_ops[*ap]);
				else
					(void)snprintf(bp, BLEFT, "(%ld",
						       (long)*ap);
				s = ", ";
				ap++;
				argsize -= sizeof(register_t);
				break;
			default :
				break;
			}
			bp += strlen(bp);
		}
		if (eol) {
			while (argsize) {
				if (!nohdr || strcmp(s, "(")) {
					if (decimal)
						(void)snprintf(bp, BLEFT,
							       "%s%ld", s,
							       (long)*ap);
					else
						(void)snprintf(bp, BLEFT,
							       "%s%#lx", s,
							       (long)*ap);
					bp += strlen(bp);
				}
				s = ", ";
				ap++;
				argsize -= sizeof(register_t);
			}
			if (BLEFT > 1)
				*bp++ = ')';
		}
	}
	*bp = '\0';

	*lenp = bp - buff;
	return eol;
}

void
ktrsysret(ktr, buff, buffsz, lenp)
	struct ktr_sysret *ktr;
	int buffsz, *lenp;
	char *buff;
{
	register register_t ret = ktr->ktr_retval;
	register int error = ktr->ktr_error;

	while (*lenp < 50)
		buff[(*lenp)++] = ' ';
	if (error == EJUSTRETURN)
		strcpy(buff + *lenp, " JUSTRETURN");
	else if (error == ERESTART)
		strcpy(buff + *lenp, " RESTART");
	else if (error) {
		sprintf(buff + *lenp, " Err#%d", error);
		if (error < MAXERRNOS && error >= -2)
			sprintf(buff + strlen(buff), " %s",errnos[error].name);
	} else
		sprintf(buff + *lenp, " = %ld", (long)ret);
	strcat(buff + *lenp, "\n");
	*lenp = 0;
	fputs(buff, stdout);
	*buff = '\0';
}

void
ktrnamei(cp, len, buff, buffsz, lenp)
	int buffsz, *lenp;
	char *cp, *buff;
{
	snprintf(buff + *lenp, buffsz - *lenp, "\"%.*s\"", len, cp);
	*lenp += strlen(buff + *lenp);
}

void
ktremul(ktr_header, cp, len, buff, buffsz, lenp)
	struct ktr_header *ktr_header;
	int buffsz, *lenp;
	char *cp, *buff;
{
	bzero(buff + *lenp, buffsz - *lenp);
	cp[len] = '\0';
	snprintf(buff + *lenp, buffsz - *lenp, "emul(%s)\n", cp);
	*lenp += strlen(buff + *lenp);

	setemul(cp, ktr_header->ktr_pid, 1);
}

void
ktrgenio(ktr, len, buff, bufsz, lenp)
	struct ktr_genio *ktr;
	int len;
	char *buff;
	int bufsz, *lenp;
{
	static int screenwidth = 0;
	register int datalen = len - sizeof (struct ktr_genio);
	register char *dp = (char *)ktr + sizeof (struct ktr_genio);
	register int col = 0;
	register int width;
	char visbuf[5], *bp = buff;

	if (*lenp < bufsz) {
		bp += *lenp;
		bzero(buff, BLEFT);
	} else
		*lenp = 0;
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
	strcpy(bp, "       \"");
	col = *lenp;
	col += 8;
	bp += 8;
	for (; datalen > 0; datalen--, dp++) {
		(void) vis(visbuf, *dp, VIS_NL|VIS_TAB|VIS_CSTYLE, *(dp+1));
		width = strlen(visbuf);
		visbuf[4] = '\0';
		if (col + width + 2 >= screenwidth)
			break;
		col += width;
		strncpy(bp, visbuf, width);
		bp += width;
		if (col + 2 >= screenwidth)
			break;
	}
	strcpy(bp, "\"\n");
	*lenp = col + 2;
}

void
ktrpsig(psig)
	struct ktr_psig *psig;
{
	(void)printf("SIG%s ", sys_signame[psig->signo]);
	if (psig->action == SIG_DFL)
		(void)printf("SIG_DFL\n");
	else {
		(void)printf("caught handler=0x%lx mask=0x%lx code=0x%x\n",
		    (u_long)psig->action, (unsigned long)psig->mask.__bits[0],
		    psig->code);
	}
}

void
ktrcsw(cs)
	struct ktr_csw *cs;
{
	(void)printf("%s %s\n", cs->out ? "stop" : "resume",
	    cs->user ? "user" : "kernel");
}

#	$NetBSD: Makefile,v 1.33 1999/07/02 05:52:14 itojun Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/4/94

PROG=	ftpd
SRCS=	conf.c ftpd.c ftpcmd.y logwtmp.c popen.c
CPPFLAGS+=-DHASSETPROCTITLE
DPADD+=	${LIBCRYPT}
LDADD+=	-lcrypt
MAN=	ftpd.8
MLINKS+=ftpd.8 ftpd.conf.5

# for `internal' ls
SRCS+=	ls.c cmp.c print.c stat_flags.c util.c
.PATH:	${.CURDIR}/../../bin/ls

CPPFLAGS+=-DINET6
.include <bsd.own.mk>

.ifdef SKEY
CPPFLAGS+=-DSKEY
DPADD+= ${LIBSKEY}
LDADD+= -lskey
.endif

.include <bsd.prog.mk>

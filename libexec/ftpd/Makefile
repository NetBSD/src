#	$NetBSD: Makefile,v 1.36 1999/12/07 05:30:53 lukem Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/4/94

SRCTOP=	../..
.include <bsd.crypto.mk>

PROG=	ftpd
SRCS=	conf.c ftpd.c ftpcmd.y logwtmp.c popen.c
CPPFLAGS+=-DHASSETPROCTITLE
DPADD+=	${LIBCRYPT} ${LIBUTIL}
LDADD+=	-lcrypt -lutil
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

.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/libexec/ftpd/Makefile.frag"
.endif

.include <bsd.prog.mk>

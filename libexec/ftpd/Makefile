#	$NetBSD: Makefile,v 1.39 1999/12/18 05:51:34 lukem Exp $
#	@(#)Makefile	8.2 (Berkeley) 4/4/94

SRCTOP=	../..
.include <bsd.crypto.mk>

PROG=	ftpd
SRCS=	conf.c ftpd.c ftpcmd.y logutmp.c logwtmp.c popen.c
CPPFLAGS+=-DHASSETPROCTITLE
DPADD+=	${LIBCRYPT} ${LIBUTIL}
LDADD+=	-lcrypt -lutil
MAN=	ftpd.conf.5 ftpusers.5 ftpd.8
MLINKS=	ftpusers.5 ftpchroot.5

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

ftpd.o: version.h

.if defined(CRYPTOPATH)
.sinclude "${CRYPTOPATH}/libexec/ftpd/Makefile.frag"
.endif

.include <bsd.prog.mk>

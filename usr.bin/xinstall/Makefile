#	$NetBSD: Makefile,v 1.26 2015/10/10 22:54:03 dholland Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>

PROG=	xinstall
SRCS=	xinstall.c getid.c metachar.c
MAN=	install.1

.PATH:		${NETBSDSRCDIR}/usr.bin/make
CPPFLAGS+=	-I${NETBSDSRCDIR}/usr.bin/make

.PATH:		${NETBSDSRCDIR}/usr.sbin/mtree
CPPFLAGS+=	-I${NETBSDSRCDIR}/usr.sbin/mtree

.if (${HOSTPROG:U} == "")
CPPFLAGS+=	-DHAVE_POSIX_SPAWN
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

COPTS.xinstall.c += -Wno-format-nonliteral


PROGNAME=install

.include <bsd.prog.mk>

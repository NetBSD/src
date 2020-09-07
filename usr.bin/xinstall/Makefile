#	$NetBSD: Makefile,v 1.27 2020/09/07 00:26:24 mrg Exp $
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

CWARNFLAGS.gcc+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>

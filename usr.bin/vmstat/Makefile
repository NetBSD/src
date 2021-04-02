#	$NetBSD: Makefile,v 1.35 2021/04/02 06:28:55 simonb Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/6/93

.include <bsd.own.mk>
PROG=	vmstat

CPPFLAGS+=-D_KMEMUSER -I${NETBSDSRCDIR}/sys
SRCS=	drvstats.c vmstat.c
MAN=	vmstat.1
DPADD=	${LIBKVM}
LDADD=	-lkvm

CWARNFLAGS.clang+=	-Wno-format-extra-args
COPTS.vmstat.c += -Wno-format-nonliteral

# sparc64 is broken!
COPTS.vmstat.c += -Wno-error=stack-protector

.include <bsd.prog.mk>

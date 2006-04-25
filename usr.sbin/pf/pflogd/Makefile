#	$NetBSD: Makefile,v 1.3 2006/04/25 18:36:44 drochner Exp $
#	$OpenBSD: Makefile,v 1.6 2003/11/20 23:23:09 avsm Exp $

.include <bsd.own.mk>		# for MKDYNAMICROOT definition

CPPFLAGS+=-I${NETBSDSRCDIR}/dist/pf/sbin/pflogd -I${NETBSDSRCDIR}/dist/libpcap

LDADD+= -lpcap -lutil
DPAPP+=	${LIBPCAP} ${LIBUTIL}

PROG=	pflogd
SRCS=	pflogd.c privsep.c privsep_fdpass.c
MAN=	pflogd.8

BINDIR=		/sbin
.if (${MKDYNAMICROOT} == "no")
LDSTATIC?=	-static
.endif

.include <bsd.prog.mk>

.PATH: ${NETBSDSRCDIR}/dist/pf/sbin/pflogd

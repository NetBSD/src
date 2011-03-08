#	$NetBSD: Makefile,v 1.6 2011/03/08 18:30:24 pooka Exp $
#

.PATH:	${.CURDIR}/../../sys/rump/librump/rumpkern

LIB=		rumpclient
USE_SHLIBDIR=	yes
MAN=		rumpclient.3

.include <bsd.own.mk>

INCS=		rumpclient.h
INCSDIR=	/usr/include/rump

CPPFLAGS+=	-DRUMP_CLIENT -I${.CURDIR} -I${.CURDIR}/../librumpuser
SRCS=		rumpclient.c
SRCS+=		rump_syscalls.c

CLEANFILES+=	srcsys

COPTS.rump_syscalls.c+= -fno-strict-aliasing

.if !make(obj) && !make(clean) && !make(cleandir)
.BEGIN:
	@rm -f srcsys
	@ln -s ${NETBSDSRCDIR}/sys/sys srcsys
.endif

.include <bsd.lib.mk>

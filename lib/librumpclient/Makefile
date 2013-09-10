#	$NetBSD: Makefile,v 1.9 2013/09/10 16:53:06 pooka Exp $
#

.PATH:	${.CURDIR}/../../sys/rump/librump/rumpkern

LIB=		rumpclient
USE_SHLIBDIR=	yes
MAN=		rumpclient.3

.include <bsd.own.mk>

INCS=		rumpclient.h
INCSDIR=	/usr/include/rump

.for lib in ${RUMPCLIENT_EXTERNAL_DPLIBS}
LIBDO.${lib}=	_external
LIBDPLIBS+=	${lib} lib
.endfor

CPPFLAGS+=	-DRUMP_CLIENT
CPPFLAGS+=	-I${.OBJDIR} -I${.CURDIR}
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

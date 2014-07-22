#	$NetBSD: Makefile,v 1.21 2014/07/22 22:41:58 justin Exp $
#

.include <bsd.own.mk>

WARNS?=		5

# rumpuser.h is in sys/rump for inclusion by kernel components
.PATH:		${.CURDIR}/../../sys/rump/include/rump

RUMPUSER_THREADS?=pthread

LIB=		rumpuser

.if   ${RUMPUSER_THREADS} == "pthread"
LIBDPLIBS+=	pthread ${.CURDIR}/../libpthread
.endif
.for lib in ${RUMPUSER_EXTERNAL_DPLIBS}
LIBDO.${lib}=	_external
LIBDPLIBS+=	${lib} lib
.endfor
CPPFLAGS+=	-DLIBRUMPUSER
#CPPFLAGS+=	-D_DIAGNOSTIC


.if   ${RUMPUSER_THREADS} == "pthread"
SRCS=		rumpuser.c rumpuser_pth.c rumpuser_bio.c
SRCS+=		rumpuser_sp.c
.elif ${RUMPUSER_THREADS} == "none"
SRCS=		rumpuser.c rumpuser_pth_dummy.c rumpuser_bio.c
SRCS+=		rumpuser_sp.c
.elif ${RUMPUSER_THREADS} == "fiber"
.if defined(RUMP_CURLWP)
.if ${RUMP_CURLWP} != "hypercall"
.error Unsupported curlwp scheme for thread model: ${RUMP_CURLWP}
.endif
.endif
SRCS=		rumpfiber.c rumpfiber_bio.c
SRCS+=		rumpfiber_sp.c
.else
.error Unsupported rumpuser threading type: ${RUMPUSER_THREADS}
.endif

SRCS+=		rumpuser_component.c rumpuser_random.c
SRCS+=		rumpuser_file.c rumpuser_mem.c

SRCS+=		rumpuser_errtrans.c rumpuser_sigtrans.c

# optional
SRCS+=		rumpuser_dl.c rumpuser_daemonize.c

INCSDIR=	/usr/include/rump
INCS=		rumpuser.h rumpuser_component.h rumpuser_port.h

MAN=		rumpuser.3

CPPFLAGS+=	-D_REENTRANT

.include <bsd.lib.mk>

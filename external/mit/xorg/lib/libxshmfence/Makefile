#	$NetBSD: Makefile,v 1.5 2016/09/27 19:18:42 joerg Exp $

.include <bsd.own.mk>

LIB=	xshmfence

XSHMFENCEDIR=	${X11SRCDIR.${LIB}}
.PATH:		${XSHMFENCEDIR}/src
SRCS=	 	xshmfence_alloc.c xshmfence_semaphore.c

INCS=	xshmfence.h
INCSDIR=${X11INCDIR}/X11

CPPFLAGS+=	-DSHMDIR=\"/var/shm\"
CPPFLAGS+=	-DHAVE_SEMAPHORE
# XXX
CPPFLAGS+=	-DLIBXSHM_PAGESIZE=4096

LDADD+=	-lrt
DPADD+=	${LIBRT}

NOMAN=	# defined

PKGDIST=	${LIB}

COPTS.xshmfence_semaphore.c += -Wno-stack-protector

.if ${MACHINE_ARCH} == "sparc" || ${COMMON_MACHINE_ARCH:U} == "sparc"
COPTS.xshmfence_semaphore.c+=	${${ACTIVE_CC} == "clang":? -Wa,-Av8plus  :}
.endif

.include <bsd.x11.mk>
.include <bsd.lib.mk>

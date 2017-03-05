#	$NetBSD: Makefile,v 1.6 2017/03/05 11:52:39 mrg Exp $

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
# XXX PTHREAD_LIBS really should be -pthread for more general
# XXX but the package doesn't really do this.
PKGCONFIG_SED_FLAGS= \
        -e "s,@XPROTO_CFLAGS@,-I${X11INCDIR},; \
	    s,@PTHREAD_LIBS@,-lpthread,"

COPTS.xshmfence_semaphore.c += -Wno-stack-protector

.if ${MACHINE_ARCH} == "sparc" || ${COMMON_MACHINE_ARCH:U} == "sparc"
COPTS.xshmfence_semaphore.c+=	${${ACTIVE_CC} == "clang":? -Wa,-Av8plus  :}
.endif

.include <bsd.x11.mk>
.include <bsd.lib.mk>

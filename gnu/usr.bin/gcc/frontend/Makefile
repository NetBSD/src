#	$NetBSD: Makefile,v 1.1 2001/06/18 16:18:35 tv Exp $

LIB=		frontend
MKLINT=		no
MKPROFILE=	no
MKPIC=		no
MKLINKLIB=	no

SRCS=		gcc.c version.c intl.c prefix.c

CPPFLAGS+=	-I${.CURDIR} -I${GCCARCH} \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*} \
		-DPREFIX=\"/usr\" \
		-DDEFAULT_TARGET_VERSION=\"${G_version}\" \
                -DDEFAULT_TARGET_MACHINE=\"${MACHINE_GNU_ARCH}-netbsd\"
HOST_CPPFLAGS+=	-I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

.include <bsd.lib.mk>

${OBJS}: multilib.h

.PATH: ${DIST}/gcc

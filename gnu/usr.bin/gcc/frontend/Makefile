#	$NetBSD: Makefile,v 1.4 2002/09/19 03:09:36 lukem Exp $

NOLINKLIB=	# defined
NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB=		frontend

SRCS=		gcc.c version.c intl.c prefix.c

CPPFLAGS+=	-I${.CURDIR} -I${GCCARCH} \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*} \
		-DPREFIX=\"/usr\" \
		-DDEFAULT_TARGET_VERSION=\"${G_version}\" \
		-DDEFAULT_TARGET_MACHINE=\"${MACHINE_GNU_PLATFORM}\"
HOST_CPPFLAGS+=	-I${GCCARCH} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

.include <bsd.lib.mk>

${OBJS}: multilib.h

.PATH: ${DIST}/gcc

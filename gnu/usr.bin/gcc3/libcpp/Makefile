#	$NetBSD: Makefile,v 1.1 2003/07/25 16:32:39 mrg Exp $

NOLINKLIB=	# defined
NOLINT=		# defined
NOPIC=		# defined
NOPROFILE=	# defined

LIB=		cpp

SRCS=		${G_LIBCPP_OBJS:.o=.c}

CPPFLAGS+=	-I${.CURDIR} -I${GCCARCH} -I${BACKENDOBJ} \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*} \
		-DPREFIX=\"/usr\"

.include <bsd.lib.mk>

${OBJS}: ${G_LIBCPP_DEPS}

.PATH: ${DIST}/gcc

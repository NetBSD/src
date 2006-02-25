#	$NetBSD: Makefile,v 1.3 2006/02/25 12:04:23 simonb Exp $

MKPRIVATELIB=	yes

LIB=		cpp

SRCS=		${G_LIBCPP_OBJS:.o=.c}

CPPFLAGS+=	-I${.CURDIR} -I${GCCARCH} -I${BACKENDOBJ} \
		${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*} \
		-DPREFIX=\"/usr\"

# XXX gcc332 gets an ICE compiling cppmacro.c with -O2.
.if ${MACHINE_ARCH} == "ns32k"
COPTS.cppmacro.c= -O1
.endif

.include <bsd.lib.mk>

${OBJS}: ${G_LIBCPP_DEPS}

.PATH: ${DIST}/gcc

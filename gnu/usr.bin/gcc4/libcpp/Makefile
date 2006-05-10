#	$NetBSD: Makefile,v 1.1 2006/05/10 22:48:14 mrg Exp $

MKPRIVATELIB=	yes

LIB=		cpp

SRCS=		${G_libcpp_a_OBJS:.o=.c}

CPPFLAGS+=	-I${.CURDIR} -I${.CURDIR}/arch/${MACHINE_ARCH} \
		-I${GCCARCH} -I${BACKENDOBJ} \
		${G_INCLUDES:M-I*:N-I.*} \
		-DPREFIX=\"/usr\"

# XXX gcc332 gets an ICE compiling cppmacro.c with -O2.
#.if ${MACHINE_ARCH} == "ns32k"
#COPTS.cppmacro.c= -O1
#.endif

.include <bsd.lib.mk>

${OBJS}: ${G_LIBCPP_DEPS}

.PATH: ${DIST}/libcpp
# ${DIST}/gcc

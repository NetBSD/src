#	$NetBSD: Makefile,v 1.1.1.1.8.2 2014/08/19 23:58:28 tls Exp $

LIBISPRIVATE=	yes

LIB=		cpp

SRCS=		${G_libcpp_a_OBJS:.o=.c}

CPPFLAGS+=	-I${.CURDIR} -I${.CURDIR}/arch/${GCC_MACHINE_ARCH} \
		-I${GCCARCH} -I${BACKENDOBJ} -I. \
		${G_INCLUDES:M-I*:N-I.*} \
		-DPREFIX=\"/usr\"

.include <bsd.lib.mk>
COPTS+=	-Wno-stack-protector

${OBJS}: ${G_LIBCPP_DEPS}

.PATH: ${DIST}/libcpp
# ${DIST}/gcc

localedir.h:
	echo '#define LOCALEDIR "/usr/share/locale"' > ${.TARGET}
CLEANFILES+= localedir.h

init.d init.o: localedir.h

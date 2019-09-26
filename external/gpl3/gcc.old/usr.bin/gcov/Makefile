#	$NetBSD: Makefile,v 1.10 2019/09/26 08:04:07 mrg Exp $

.include <bsd.init.mk>

PROG=           gcov
SRCS=		${G_GCOV_OBJS:.o=.c}

CPPFLAGS+=	-I${GCCARCH} -I${BACKENDOBJ} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

BINDIR=		/usr/bin

MAN=		gcov.1

.include "../Makefile.prog"

.include "../Makefile.common"
.include "../Makefile.libcpp"
.include "../Makefile.backtrace"
.include "../Makefile.libiberty"
.include "../Makefile.libdecnumber"

COPTS.gcov.c += -Wno-stack-protector

LDADD+=		-lintl
DPADD+=		${LIBINTL}

.PATH: ${DIST}/gcc ${DIST}/gcc/doc

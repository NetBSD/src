#	$NetBSD: Makefile,v 1.1.1.1.4.3 2020/04/13 08:02:05 martin Exp $

.include <bsd.init.mk>

PROG=           gcov-dump
SRCS=		${G_GCOV_DUMP_OBJS:.o=.c}

CPPFLAGS+=	-I${GCCARCH} -I${BACKENDOBJ} ${G_ALL_CFLAGS:M-D*} ${G_INCLUDES:M-I*:N-I.*}

BINDIR=		/usr/bin

MAN=		gcov-dump.1

.include "../Makefile.common"
.include "../Makefile.libcpp"
.include "../Makefile.backtrace"
.include "../Makefile.libiberty"
.include "../Makefile.libdecnumber"

.include "../Makefile.prog"

LDADD+=		-lintl
DPADD+=		${LIBINTL}

.PATH: ${DIST}/gcc ${DIST}/gcc/doc

# $FreeBSD: src/cddl/lib/drti/Makefile,v 1.1.4.1 2009/08/03 08:13:06 kensmith Exp $
.include	<bsd.init.mk>
.include	"../../Makefile.inc"

#.if ${MKPIC} == "yes"
CFLAGS+=-fPIC
#.endif

.PATH:		${OPENSOLARIS_USR_DISTDIR}/lib/libdtrace/common

SRCS=		drti.c
OBJS=		${SRCS:R:S/$/.o/g}
FILES=		${OBJS}
FILESOWN=	${LIBOWN}
FILESGRP=	${LIBGRP}
FILESMODE=	${LIBMODE}
FILESDIR=	${LIBDIR}/dtrace
WARNS?=		6
CLEANFILES=	${FILES}

CPPFLAGS+=	-I${.CURDIR}/../../dist/lib/libdtrace/common \
		-I${.CURDIR}/../../include \
		-I${.CURDIR}/../../sys  \
		-I${.CURDIR}/../../dist/uts/common

realall: ${OBJS}

.include <bsd.prog.mk>

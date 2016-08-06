# $NetBSD: Makefile,v 1.5.2.1 2016/08/06 00:18:41 pgoyette Exp $
.include	<bsd.init.mk>

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

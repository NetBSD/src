#	$NetBSD: Makefile.inc,v 1.13 2011/05/26 12:56:35 joerg Exp $

WARNS?=	1	# XXX -Wshadow -Wcast-qual -Wsign-compare

.include <bsd.own.mk>

USE_FORT?=yes	# network client/server

CPPFLAGS+=-UCMUCS -UCMU -UMACH -DVAR_TMP -DHAS_DAEMON -DHAS_POSIX_DIR \
    -DLIBWRAP -DHAS_FPARSELN -DHAS_VIS -DHAS_FGETLN
CPPFLAGS+=-I${.CURDIR}/../source -I${.CURDIR}/../lib -I${.CURDIR}/../sys
CPPFLAGS+=-DEE_XXX

CWARNFLAGS+=-Wextra -Wno-sign-compare

LIBSUPDIR != cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
LIBSUP=	${LIBSUPDIR}/libsup.a
SUPLIB=	-L${LIBSUPDIR} -lsup
LDADD+=-lutil
DPADD+=${LIBUTIL}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

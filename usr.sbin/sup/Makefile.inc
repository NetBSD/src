#	$NetBSD: Makefile.inc,v 1.7 2001/01/07 08:00:57 enami Exp $

.include <bsd.own.mk>

CPPFLAGS+=-UCMUCS -UCMU -UMACH -DVAR_TMP -DHAS_DAEMON -DHAS_POSIX_DIR \
    -DLIBWRAP -DHAS_FPARSELN
CPPFLAGS+=-I${.CURDIR}/../source -I${.CURDIR}/../lib -I${.CURDIR}/../sys
CPPFLAGS+=-DEE_XXX

LIBSUPDIR != cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
LIBSUP=	${LIBSUPDIR}/libsup.a
SUPLIB=	-L${LIBSUPDIR} -lsup

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

#	$NetBSD: Makefile.inc,v 1.8 2003/04/03 17:56:52 christos Exp $

.include <bsd.own.mk>

CPPFLAGS+=-UCMUCS -UCMU -UMACH -DVAR_TMP -DHAS_DAEMON -DHAS_POSIX_DIR \
    -DLIBWRAP -DHAS_FPARSELN -DHAS_VIS -DHAS_FGETLN
CPPFLAGS+=-I${.CURDIR}/../source -I${.CURDIR}/../lib -I${.CURDIR}/../sys
CPPFLAGS+=-DEE_XXX

LIBSUPDIR != cd ${.CURDIR}/../lib && ${PRINTOBJDIR}
LIBSUP=	${LIBSUPDIR}/libsup.a
SUPLIB=	-L${LIBSUPDIR} -lsup

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

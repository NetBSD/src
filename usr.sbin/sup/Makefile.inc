#	$NetBSD: Makefile.inc,v 1.2.2.1 1997/11/08 15:12:47 lukem Exp $

CPPFLAGS+=-UCMUCS -UCMU -UMACH -DVAR_TMP -DHAS_DAEMON -DHAS_POSIX_DIR
CPPFLAGS+=-I${.CURDIR}/../source -I${.CURDIR}/../lib -I${.CURDIR}/../sys

LIBSUPDIR != cd ${.CURDIR}/../lib;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
LIBSUP=	${LIBSUPDIR}/libsup.a
SUPLIB=	-L${LIBSUPDIR} -lsup

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

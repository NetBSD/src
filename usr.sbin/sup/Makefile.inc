#	$NetBSD: Makefile.inc,v 1.1 1997/10/07 01:31:11 thorpej Exp $

CFLAGS+=-UCMUCS -UCMU -UMACH -DVAR_TMP -DHAS_DAEMON -DHAS_POSIX_DIR
CFLAGS+=-I${.CURDIR}/../source -I${.CURDIR}/../lib -I${.CURDIR}/../sys

LIBSUPDIR != cd ${.CURDIR}/../lib;\
    printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-
LIBSUP=	${LIBSUPDIR}/libsup.a
SUPLIB=	-L${LIBSUPDIR} -lsup

.if exists(${CURDIR}/../../Makefile.inc)
.include "${CURDIR}/../../Makefile.inc
.endif

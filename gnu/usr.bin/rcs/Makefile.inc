#	$NetBSD: Makefile.inc,v 1.8 1997/10/18 04:42:04 lukem Exp $

LIBRCS!=cd $(.CURDIR)/../lib; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/librcs.a\n" | ${MAKE} -s -f-
CFLAGS+=-I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} ${LIBGNUMALLOC}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

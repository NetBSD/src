#	$NetBSD: Makefile.inc,v 1.8.2.1 1997/11/08 06:54:15 lukem Exp $

LIBRCS!=cd $(.CURDIR)/../lib; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/librcs.a\n" | ${MAKE} -s -f-
CPPFLAGS+=-I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} ${LIBGNUMALLOC}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

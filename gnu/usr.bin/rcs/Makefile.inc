#	$NetBSD: Makefile.inc,v 1.9 1997/10/25 10:12:48 lukem Exp $

LIBRCS!=cd $(.CURDIR)/../lib; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/librcs.a\n" | ${MAKE} -s -f-
CPPFLAGS+=-I${.CURDIR}/../lib
LDADD=	${LIBRCS} -lgnumalloc
DPADD=	${LIBRCS} ${LIBGNUMALLOC}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

#	$NetBSD: Makefile.inc,v 1.10 1999/07/30 04:07:26 mycroft Exp $

LIBRCS!=cd $(.CURDIR)/../lib; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}/librcs.a\n" | ${MAKE} -s -f-
CPPFLAGS+=-I${.CURDIR}/../lib
LDADD=	${LIBRCS}
DPADD=	${LIBRCS}

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

#	$NetBSD: Makefile.inc,v 1.1 1999/02/09 13:18:58 simonb Exp $

DIST=	${.CURDIR}/../../../dist/grep
.PATH: ${DIST}/src
.PATH: ${DIST}/doc

#COBJDIR!= cd ${.CURDIR}/../common ; make print-objdir
COBJDIR!=cd $(.CURDIR)/../common; \
	printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

CPPFLAGS+=-I${.CURDIR}/../common
DPADD+= ${COBJDIR}/libgrep.a
LDADD+= -L${COBJDIR} -lgrep

.if exists(${.CURDIR}/../../Makefile.inc)
.include "${.CURDIR}/../../Makefile.inc"
.endif

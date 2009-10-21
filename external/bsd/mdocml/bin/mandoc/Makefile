# $NetBSD: Makefile,v 1.1 2009/10/21 18:04:52 joerg Exp $

.include <bsd.own.mk>

PROG=		mandoc

SRCS=		main.c mdoc_term.c chars.c term.c tree.c compat.c \
	        man_term.c html.c mdoc_html.c man_html.c out.c

.ifndef HOSTPROG
DPADD+= 	${MDOCMLLIB.man} ${MDOCMLLIB.mdoc}
LDADD+= 	-L${MDOCMLOBJDIR.man} -lman -L${MDOCMLOBJDIR.mdoc} -lmdoc
.else
SRCS.libman!=	cd ${.PARSEDIR}/../../lib/libman && ${MAKE} -V '$${SRCS}'
SRCS.libmdoc!=	cd ${.PARSEDIR}/../../lib/libmdoc && ${MAKE} -V '$${SRCS}'

SRCS+=		${SRCS.libman} ${SRCS.libmdoc:Nmandoc.c}
.endif

.include <bsd.prog.mk>

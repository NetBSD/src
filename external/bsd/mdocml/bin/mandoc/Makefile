# $NetBSD: Makefile,v 1.3 2010/06/24 17:20:19 joerg Exp $

.include <bsd.own.mk>

PROG=		mandoc

SRCS=		main.c mdoc_term.c chars.c term.c term_ascii.c term_ps.c \
		tree.c compat.c \
	        man_term.c html.c mdoc_html.c man_html.c out.c

.ifndef HOSTPROG
DPADD+= 	${MDOCMLLIB.man} ${MDOCMLLIB.mdoc} ${MDOCMLLIB.roff}
LDADD+= 	-L${MDOCMLOBJDIR.man} -lman \
		-L${MDOCMLOBJDIR.mdoc} -lmdoc \
		-L${MDOCMLOBJDIR.roff} -lroff
.else
SRCS.libman!=	cd ${.PARSEDIR}/../../lib/libman && ${MAKE} -V '$${SRCS}'
SRCS.libmdoc!=	cd ${.PARSEDIR}/../../lib/libmdoc && ${MAKE} -V '$${SRCS}'
SRCS.libroff!=	cd ${.PARSEDIR}/../../lib/libroff && ${MAKE} -V '$${SRCS}'

SRCS+=		${SRCS.libman} ${SRCS.libmdoc:Nmandoc.c} ${SRCS.libroff}
.endif

.include <bsd.prog.mk>

# $NetBSD: Makefile,v 1.10 2015/12/18 14:33:54 christos Exp $

.include <bsd.own.mk>

PROG=		mandoc

SRCS=		main.c out.c tree.c \
		eqn_html.c eqn_term.c \
		html.c man_html.c mdoc_html.c tbl_html.c \
		manpath.c man_term.c mdoc_term.c term.c term_ascii.c \
		term_ps.c tbl_term.c

.ifndef HOSTPROG
DPADD+= 	${MDOCMLLIB.mandoc}
LDADD+= 	-L${MDOCMLOBJDIR.mandoc} -lmandoc
CPPFLAGS+=	-DUSE_WCHAR
.else
SRCS.libmandoc!=cd ${.PARSEDIR}/../../lib/libmandoc && ${MAKE} -V '$${SRCS}'

SRCS+=		${SRCS.libmandoc} compat_strtonum.c compat_reallocarray.c
.endif

.include <bsd.prog.mk>

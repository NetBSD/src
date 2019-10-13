# $NetBSD: Makefile,v 1.19 2019/10/13 07:28:06 mrg Exp $

.include <bsd.own.mk>

PROG=		mandoc

SRCS=	\
eqn_html.c \
eqn_term.c \
html.c \
dba.c \
dba_array.c \
dba_read.c \
dba_write.c \
dbm.c \
dbm_map.c \
main.c \
man_html.c \
man_term.c \
mandocdb.c \
manpath.c \
mansearch.c \
mdoc_html.c \
mdoc_markdown.c \
mdoc_term.c \
out.c \
roff_html.c \
roff_term.c \
tbl_html.c \
tbl_term.c \
term.c \
term_ascii.c \
term_ps.c \
term_tab.c \
tree.c

.ifndef HOSTPROG
DPADD+= 	${MDOCMLLIB.mandoc} ${LIBZ}
LDADD+= 	-L${MDOCMLOBJDIR.mandoc} -lmandoc -lz
CPPFLAGS+=	-DUSE_WCHAR
.else
SRCS.libmandoc!=cd ${.PARSEDIR}/../../lib/libmandoc && ${MAKE} -v SRCS

SRCS+=		${SRCS.libmandoc} compat_strtonum.c compat_reallocarray.c
.endif

COPTS.man_term.c+= -Wno-error=array-bounds

COPTS.tbl_term.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>

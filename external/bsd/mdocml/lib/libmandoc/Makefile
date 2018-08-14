# $NetBSD: Makefile,v 1.9 2018/08/14 08:53:06 christos Exp $

LIBISPRIVATE=	yes

LIB=	mandoc
SRCS=	\
att.c \
chars.c \
compat_ohash.c \
eqn.c \
lib.c \
man.c \
man_macro.c \
man_validate.c \
mandoc.c \
mandoc_aux.c \
mandoc_ohash.c \
mdoc.c \
mdoc_argv.c \
mdoc_macro.c \
mdoc_man.c \
mdoc_state.c \
mdoc_validate.c \
msec.c \
preconv.c \
read.c \
roff.c \
roff_validate.c \
st.c \
tag.c \
tbl.c \
tbl_data.c \
tbl_layout.c \
tbl_opts.c

MAN=	mandoc.3

# XXX
.if ${MACHINE} == "vax"
COPTS.mdoc_macro.c+=-O0
.endif

.include <bsd.lib.mk>

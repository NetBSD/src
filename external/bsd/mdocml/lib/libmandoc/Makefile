# $NetBSD: Makefile,v 1.7.2.1 2017/04/21 16:52:02 bouyer Exp $

LIBISPRIVATE=	yes

LIB=	mandoc
SRCS=	\
att.c \
chars.c \
compat_ohash.c \
eqn.c \
lib.c \
man.c \
man_hash.c \
man_macro.c \
man_validate.c \
mandoc.c \
mandoc_aux.c \
mandoc_ohash.c \
mdoc.c \
mdoc_argv.c \
mdoc_hash.c \
mdoc_macro.c \
mdoc_man.c \
mdoc_state.c \
mdoc_validate.c \
msec.c \
preconv.c \
read.c \
roff.c \
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

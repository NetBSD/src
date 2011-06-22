# $NetBSD: Makefile,v 1.2 2011/06/22 02:49:42 mrg Exp $

LIBISPRIVATE=	yes

LIB=	mandoc
SRCS=	man.c man_hash.c man_macro.c man_validate.c \
	arch.c att.c lib.c mdoc.c mdoc_argv.c mdoc_hash.c mdoc_macro.c \
	mdoc_validate.c msec.c st.c vol.c \
	eqn.c roff.c tbl.c tbl_data.c tbl_layout.c tbl_opts.c \
	mandoc.c read.c
MAN=	mandoc.3

.include <bsd.lib.mk>

# XXX
.if ${HAVE_GCC} == 45
COPTS.mdoc_validate.c+=	-Wno-error
.endif

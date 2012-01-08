# $NetBSD: Makefile,v 1.5 2012/01/08 21:34:21 rmind Exp $

PROG=		npfctl
MAN=		npfctl.8 npf.conf.5

SRCS=		npfctl.c npf_var.c npf_data.c npf_ncgen.c npf_build.c

CPPFLAGS+=	-I${.CURDIR}
SRCS+=		npf_scan.l npf_parse.y
YHEADER=	1

LDADD+=		-lnpf -lprop -ly
DPADD+=		${LIBNPF} ${LIBPROP}

WARNS?=		4
NOLINT=		# disabled (note: deliberately)

.include <bsd.prog.mk>

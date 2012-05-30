# $NetBSD: Makefile,v 1.7 2012/05/30 21:30:07 rmind Exp $

PROG=		npfctl
MAN=		npfctl.8 npf.conf.5

SRCS=		npfctl.c npf_var.c npf_data.c npf_ncgen.c npf_build.c \
		npf_disassemble.c

CPPFLAGS+=	-I${.CURDIR}
SRCS+=		npf_scan.l npf_parse.y
YHEADER=	1

LDADD+=		-lnpf -lprop -lutil -ly
DPADD+=		${LIBNPF} ${LIBPROP} ${LIBUTIL}

WARNS?=		4
NOLINT=		# disabled (note: deliberately)

.include <bsd.prog.mk>

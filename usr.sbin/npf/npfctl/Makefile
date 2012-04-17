# $NetBSD: Makefile,v 1.4.4.1 2012/04/17 00:09:50 yamt Exp $

PROG=		npfctl
MAN=		npfctl.8 npf.conf.5

SRCS=		npfctl.c npf_var.c npf_data.c npf_ncgen.c npf_build.c \
		npf_disassemble.c

CPPFLAGS+=	-I${.CURDIR}
SRCS+=		npf_scan.l npf_parse.y
YHEADER=	1
YFLAGS+=	-v

LDADD+=		-lnpf -lprop -lutil -ly
DPADD+=		${LIBNPF} ${LIBPROP} ${LIBUTIL}

WARNS?=		4
NOLINT=		# disabled (note: deliberately)

.include <bsd.prog.mk>

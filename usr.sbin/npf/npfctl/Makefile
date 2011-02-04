# $NetBSD: Makefile,v 1.4 2011/02/04 00:19:51 rmind Exp $

PROG=		npfctl
MAN=		npfctl.8 npf.conf.5

SRCS=		npfctl.c npf_parser.c npf_data.c npf_ncgen.c

LDADD+=		-lnpf -lprop
DPADD+=		${LIBNPF} ${LIBPROP}

WARNS?=		4
NOLINT=		# defined (note: deliberately)

.include <bsd.prog.mk>

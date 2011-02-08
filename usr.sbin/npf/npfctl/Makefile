# $NetBSD: Makefile,v 1.2.2.1 2011/02/08 16:20:15 bouyer Exp $

PROG=		npfctl
MAN=		npfctl.8 npf.conf.5

SRCS=		npfctl.c npf_parser.c npf_data.c npf_ncgen.c

LDADD+=		-lnpf -lprop
DPADD+=		${LIBNPF} ${LIBPROP}

WARNS?=		4
NOLINT=		# defined (note: deliberately)

.include <bsd.prog.mk>

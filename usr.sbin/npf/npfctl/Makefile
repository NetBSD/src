# $NetBSD: Makefile,v 1.2 2010/08/24 23:55:04 rmind Exp $

PROG=		npfctl
MAN=		npfctl.8 npf.conf.5

SRCS=		npfctl.c npf_parser.c npf_data.c npf_ncgen.c

LDADD+=		-lprop
DPADD+=		${LIBPROP}

WARNS?=		4
NOLINT=		# defined (note: deliberately)

.include <bsd.prog.mk>

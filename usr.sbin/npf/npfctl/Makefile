# $NetBSD: Makefile,v 1.1 2010/08/22 18:56:23 rmind Exp $

PROG=		npfctl
MAN=		npfctl.8 npf.conf.8

SRCS=		npfctl.c npf_parser.c npf_data.c npf_ncgen.c

LDADD+=		-lprop
DPADD+=		${LIBPROP}

WARNS?=		4
NOLINT=		# defined (note: deliberately)

.include <bsd.prog.mk>

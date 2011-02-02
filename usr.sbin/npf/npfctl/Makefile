# $NetBSD: Makefile,v 1.3 2011/02/02 02:20:25 rmind Exp $

PROG=		npfctl
MAN=		npfctl.8 npf.conf.5

SRCS=		npfctl.c npf_parser.c npf_data.c npf_ncgen.c

LDADD+=		-lnpf
DPADD+=		${LIBNPF}

WARNS?=		4
NOLINT=		# defined (note: deliberately)

.include <bsd.prog.mk>

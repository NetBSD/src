# $NetBSD: Makefile,v 1.11 2013/09/19 01:04:45 rmind Exp $

PROG=		npfctl
MAN=		npfctl.8 npf.conf.5
BINDIR=		/sbin

SRCS=		npfctl.c npf_var.c npf_data.c npf_build.c npf_extmod.c
SRCS+=		npf_bpf_comp.c npf_show.c

CPPFLAGS+=	-I${.CURDIR}
SRCS+=		npf_scan.l npf_parse.y
YHEADER=	1

LDADD+=		-lnpf -lprop -lcrypto -lpcap -lutil -ly
DPADD+=		${LIBNPF} ${LIBPROP} ${LIBUTIL}

WARNS=		5
NOLINT=		# disabled deliberately

.include <bsd.prog.mk>

# $NetBSD: Makefile,v 1.13.2.2 2020/06/20 15:46:48 martin Exp $

.include <bsd.own.mk>

PROG=		npfctl
MAN=		npfctl.8 npf.conf.5
BINDIR=		/sbin

SRCS=		npfctl.c npf_cmd.c npf_var.c npf_data.c npf_build.c
SRCS+=		npf_bpf_comp.c npf_show.c npf_extmod.c

CPPFLAGS+=	-I${.CURDIR}
SRCS+=		npf_scan.l npf_parse.y
YHEADER=	1

LDADD+=		-lnpf -lpcap -lutil -ly
DPADD+=		${LIBNPF} ${LIBUTIL} ${LIBPCAP} ${LIBUTIL} ${LIBY}

WARNS=		5
NOLINT=		# disabled deliberately

.include <bsd.prog.mk>

# $NetBSD: Makefile,v 1.16 2023/09/14 20:03:56 rillig Exp $

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

.include <bsd.prog.mk>

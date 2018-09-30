# $NetBSD: Makefile,v 1.12.14.1 2018/09/30 01:46:01 pgoyette Exp $

.include <bsd.own.mk>

PROG=		npfctl
MAN=		npfctl.8 npf.conf.5
BINDIR=		/sbin

SRCS=		npfctl.c npf_var.c npf_data.c npf_build.c npf_extmod.c
SRCS+=		npf_bpf_comp.c npf_show.c

CPPFLAGS+=	-I${.CURDIR}
SRCS+=		npf_scan.l npf_parse.y
YHEADER=	1

PROGDPLIBS+=	nv ${NETBSDSRCDIR}/external/bsd/libnv/lib
CPPFLAGS+=	-I ${NETBSDSRCDIR}/sys/external/bsd/libnv/dist

LDADD+=		-lnpf -lpcap -lutil -ly
DPADD+=		${LIBNPF} ${LIBUTIL} ${LIBPCAP} ${LIBUTIL} ${LIBY}

WARNS=		5
NOLINT=		# disabled deliberately

.include <bsd.prog.mk>

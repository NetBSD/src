#	$NetBSD: Makefile,v 1.7 1999/03/22 02:53:22 mycroft Exp $

.include <bsd.own.mk>

MAN=	mkfilters.1
.if ${MKSHARE} != "no"
FILESDIR= /usr/share/examples/ipf

FILES=	BASIC.NAT BASIC_1.FW BASIC_2.FW example.1 example.2 example.3 \
	example.4 example.5 example.6 example.7 example.8 example.9 \
	example.10 example.sr firewall ftp-proxy ftppxy nat-setup \
	nat.eg server tcpstate mkfilters mediaone
.endif

.include <bsd.man.mk>
.include <bsd.files.mk>

#	$NetBSD: Makefile,v 1.5 1999/02/13 02:54:59 lukem Exp $

.include <bsd.own.mk>

.if ${MKSHARE} != "no"
FILESDIR= /usr/share/examples/ipf
MAN=	mkfilters.1

FILES=	BASIC.NAT BASIC_1.FW BASIC_2.FW example.1 example.2 example.3 \
	example.4 example.5 example.6 example.7 example.8 example.9 \
	example.10 example.sr firewall ftp-proxy ftppxy nat-setup \
	nat.eg server tcpstate mkfilters
.endif

.include <bsd.own.mk>
.include <bsd.man.mk>
.include <bsd.files.mk>

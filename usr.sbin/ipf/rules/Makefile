#	$NetBSD: Makefile,v 1.9 2002/01/26 09:30:53 martti Exp $

.include <bsd.own.mk>

MAN=	mkfilters.1
.if ${MKSHARE} != "no"
FILESDIR= /usr/share/examples/ipf

FILES=	BASIC.NAT BASIC_1.FW BASIC_2.FW example.1 example.2 example.3 \
	example.4 example.5 example.6 example.7 example.8 example.9 \
	example.10 example.11 example.12 example.13 example.sr \
	firewall ftp-proxy ftppxy mediaone nat-setup \
	nat.eg server tcpstate mkfilters
.endif

.PATH:  ${.CURDIR}/../../../dist/ipf/rules
.include <bsd.man.mk>
.include <bsd.files.mk>

#	$NetBSD: Makefile,v 1.1.20.2 2017/12/03 11:38:53 jdolecek Exp $

.include "../Makefile.inc"

#S?=	/usr/src/sys

KMOD=	ping
SRCS=	ping.c

.include <bsd.kmodule.mk>

# To make use of this module, you'll need to separately build the
# cmd_ping program, with a Makefile similar to
#
#	MKMAN=	NO
#	PROG=	cmd_ping
#	.include <bsd.prog.mk>


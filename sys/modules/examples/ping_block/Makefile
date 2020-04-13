#	$NetBSD: Makefile,v 1.1.6.2 2020/04/13 08:05:11 martin Exp $

.include "../Makefile.inc"

#S?=	/usr/src/sys

KMOD=	ping_block
SRCS=	ping.c

.include <bsd.kmodule.mk>

# To make use of this module, you'll need to separately build the
# cmd_ping program, with a Makefile similar to
#
#	MKMAN=	NO
#	PROG=	cmd_ping
#	.include <bsd.prog.mk>


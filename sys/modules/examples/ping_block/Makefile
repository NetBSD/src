#	$NetBSD: Makefile,v 1.1 2020/02/05 13:23:42 kamil Exp $

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


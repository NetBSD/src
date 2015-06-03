#	$NetBSD: Makefile,v 1.1.2.2 2015/06/03 06:51:40 snj Exp $

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


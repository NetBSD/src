#	$NetBSD: Makefile,v 1.1 2019/01/17 20:47:42 kamil Exp $

.include "../Makefile.inc"

#S?=	/usr/src/sys

KMOD=	mapper
SRCS=	mapper.c

.include <bsd.kmodule.mk>

# To make use of this module, you'll need to separately build the
# cmd_mapper program, with a Makefile similar to
#
#	MKMAN=	NO
#	PROG=	cmd_mapper
#	.include <bsd.prog.mk>

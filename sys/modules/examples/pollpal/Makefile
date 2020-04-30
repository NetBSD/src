#   $NetBSD: Makefile,v 1.1 2020/04/30 00:48:10 christos Exp $

.include "../Makefile.inc"

#S?=	/usr/src/sys

KMOD=   pollpal
SRCS=   pollpal.c

.include <bsd.kmodule.mk>

# To make use of this module, you'll need to separately build the
# cmd_pollpall program, with a Makefile similar to
#
#   MKMAN=  NO
#   PROG=   cmd_pollpal
#   .include <bsd.prog.mk>


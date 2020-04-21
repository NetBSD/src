#	$NetBSD: Makefile,v 1.11.78.2 2020/04/21 19:37:34 martin Exp $

.include <bsd.own.mk>

USE_FORT?=yes	# setuid
# XXX Unsupported Kerberos options were removed from man page
# XXX Don't forget to update the man page if you fix Kerberos
PROG=	rcmd
SRCS=	rsh.c getport.c
CPPFLAGS+=-DIN_RCMD
BINOWN=	root
BINMODE=4555
.PATH:	${NETBSDSRCDIR}/usr.bin/rsh

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.8 2004/10/16 09:58:38 dan Exp $

.include <bsd.own.mk>

# XXX Unsupported Kerberos options were removed from man page
# XXX Don't forget to update the man page if you fix Kerberos
PROG=	rcmd
SRCS=	rsh.c getport.c
CPPFLAGS+=-DIN_RCMD
BINOWN=	root
BINMODE=4555
.PATH:	${NETBSDSRCDIR}/usr.bin/rsh

.include <bsd.prog.mk>

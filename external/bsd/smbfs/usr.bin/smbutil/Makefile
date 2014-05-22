#	$NetBSD: Makefile,v 1.1.4.2 2014/05/22 15:51:14 yamt Exp $

WARNS?=	3	# XXX: sign-compare issues

BINDIR=/usr/bin

.include "../../Makefile.inc"

.PATH: 
PROG=	smbutil
MAN=	smbutil.1
SRCS=	dumptree.c login.c lookup.c print.c smbutil.c view.c      

.PATH:  ${SMBDIST}/smbutil

LDADD+=-lutil
DPADD+=${LIBUTIL}
PROGDPLIBS+= smb ${.CURDIR}/../../lib/libsmb

.include <bsd.prog.mk>

# bootp/bootpd/Makefile
# $Id: Makefile,v 1.2 1994/05/30 05:01:43 cgd Exp $

PROG=	bootpd

SRCS=	bootpd.c dovend.c readfile.c hash.c dumptab.c \
	 lookup.c getif.c hwaddr.c report.c tzone.c

MAN5=	bootptab.0
MAN8=	bootpd.0
MLINKS= bootpd.8 bootpgw.8

# XXXX
BINDIR?= /usr/libexec

.include <bsd.prog.mk>

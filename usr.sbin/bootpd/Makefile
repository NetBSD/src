# bootpd/Makefile
# $NetBSD: Makefile,v 1.3.2.1 1997/11/08 15:11:16 lukem Exp $

WARNS?=	0
PROG=	bootpd
CPPFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG -DVEND_CMU

SRCS=	bootpd.c dovend.c readfile.c hash.c dumptab.c \
	 lookup.c getif.c hwaddr.c report.c tzone.c

MAN=	bootpd.8 bootptab.5
MLINKS= bootpd.8 bootpgw.8

.include <bsd.prog.mk>

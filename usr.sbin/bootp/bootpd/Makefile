# bootpd/Makefile
# $NetBSD: Makefile,v 1.3 1997/10/18 04:36:50 lukem Exp $

WARNS?=	0
PROG=	bootpd
CFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG -DVEND_CMU

SRCS=	bootpd.c dovend.c readfile.c hash.c dumptab.c \
	 lookup.c getif.c hwaddr.c report.c tzone.c

MAN=	bootpd.8 bootptab.5
MLINKS= bootpd.8 bootpgw.8

.include <bsd.prog.mk>

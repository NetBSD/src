# bootpd/Makefile
# $Id: Makefile,v 1.2 1994/12/22 11:33:08 cgd Exp $

PROG=	bootpd
CFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG -DVEND_CMU

SRCS=	bootpd.c dovend.c readfile.c hash.c dumptab.c \
	 lookup.c getif.c hwaddr.c report.c tzone.c

MAN=	bootpd.8 bootptab.5
MLINKS= bootpd.8 bootpgw.8

.include <bsd.prog.mk>

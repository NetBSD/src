# bootp/Makefile.inc
# $Id: Makefile.inc,v 1.1.1.1 1994/05/27 21:46:35 gwr Exp $

.PATH: ${.CURDIR}/..

CFLAGS+= -DETC_ETHERS -DSYSLOG -DDEBUG -DVEND_CMU -I..


# $NetBSD: Makefile.inc,v 1.4.4.2 2020/04/21 19:37:55 martin Exp $

BINDIR=/usr/bin

LDADD+= ${LIBKRB5_LDADD}
DPADD+= ${LIBKRB5_DPADD}

LDADD+= -lheimbase ${LIBVERS}
DPADD+= ${LIBHEIMBASE} ${LIBVERS}

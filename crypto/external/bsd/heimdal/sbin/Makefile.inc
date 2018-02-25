# $NetBSD: Makefile.inc,v 1.5 2018/02/25 00:16:48 mrg Exp $

BINDIR=/usr/sbin

LDADD+= ${LIBKRB5_LDADD}
DPADD+= ${LIBKRB5_DPADD}

LDADD+= -lheimbase ${LIBVERS}
DPADD+= ${LIBHEIMBASE} ${LIBVERS}

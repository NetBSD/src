# $NetBSD: Makefile.inc,v 1.6 2018/02/25 00:16:48 mrg Exp $

BINDIR=/usr/libexec

LDADD+= ${LIBKRB5_LDADD}
DPADD+= ${LIBKRB5_DPADD}

LDADD+= -lheimntlm ${LIBVERS}
DPADD+= ${LIBHEIMNTLM} ${LIBVERS}

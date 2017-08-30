# $NetBSD: Makefile.inc,v 1.2.6.1 2017/08/30 07:10:48 snj Exp $

BINDIR=/usr/bin

LDADD+= -lkrb5 -lhx509 -lasn1 -lroken -lcom_err -lwind
LDADD+= -lheimbase ${LIBVERS}
LDADD+= -lcrypto -lcrypt
LDADD+= -lsqlite3

DPADD+= ${LIBKRB5} ${LIBHX509} ${LIBASN1} ${LIBROKEN} ${LIBCOM_ERR} ${LIBWIND}
DPADD+= ${LIBHEIMBASE} ${LIBVERS}
DPADD+= ${LIBCRYPTO} ${LIBCRYPT}
DPADD+= ${LIBSQLITE3}

# $NetBSD: Makefile.inc,v 1.3 2017/01/28 21:31:53 christos Exp $

BINDIR=/usr/sbin

LDADD+= -lkrb5 -lhx509 -lasn1 -lwind
LDADD+= -lcom_err -lroken -lheimbase ${LIBVERS}
LDADD+= -lcrypto -lcrypt
LDADD+= -lsqlite3

DPADD+= ${LIBKRB5} ${LIBHX509} ${LIBASN1} ${LIBWIND}
DPADD+= ${LIBCOM_ERR} ${LIBROKEN} ${LIBHEIMBASE} ${LIBVERS}
DPADD+= ${LIBCRYPTO} ${LIBCRYPT}
DPADD+= ${LIBSQLITE3}

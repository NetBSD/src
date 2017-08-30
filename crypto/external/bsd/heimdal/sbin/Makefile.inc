# $NetBSD: Makefile.inc,v 1.2.20.1 2017/08/30 06:57:47 snj Exp $

BINDIR=/usr/sbin

LDADD+= -lkrb5 -lhx509 -lasn1 -lwind
LDADD+= -lcom_err -lroken -lheimbase ${LIBVERS}
LDADD+= -lcrypto -lcrypt
LDADD+= -lsqlite3 -lutil

DPADD+= ${LIBKRB5} ${LIBHX509} ${LIBASN1} ${LIBWIND}
DPADD+= ${LIBCOM_ERR} ${LIBROKEN} ${LIBHEIMBASE} ${LIBVERS}
DPADD+= ${LIBCRYPTO} ${LIBCRYPT}
DPADD+= ${LIBSQLITE3} ${LIBUTIL}

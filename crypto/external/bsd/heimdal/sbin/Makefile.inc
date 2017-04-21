# $NetBSD: Makefile.inc,v 1.2.32.1 2017/04/21 16:50:55 bouyer Exp $

BINDIR=/usr/sbin

LDADD+= -lkrb5 -lhx509 -lasn1 -lwind
LDADD+= -lcom_err -lroken -lheimbase ${LIBVERS}
LDADD+= -lcrypto -lcrypt
LDADD+= -lsqlite3 -lutil

DPADD+= ${LIBKRB5} ${LIBHX509} ${LIBASN1} ${LIBWIND}
DPADD+= ${LIBCOM_ERR} ${LIBROKEN} ${LIBHEIMBASE} ${LIBVERS}
DPADD+= ${LIBCRYPTO} ${LIBCRYPT}
DPADD+= ${LIBSQLITE3} ${LIBUTIL}

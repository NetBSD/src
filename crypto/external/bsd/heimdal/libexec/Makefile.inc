# $NetBSD: Makefile.inc,v 1.3.12.1 2017/04/21 16:50:55 bouyer Exp $

BINDIR=/usr/libexec

LDADD+= -lheimntlm -lkrb5 -lhx509 -lheimbase
LDADD+= -lasn1 -lcom_err -lroken ${LIBVERS}
LDADD+= -lwind
LDADD+= -lcrypto -lcrypt
LDADD+= -lsqlite3 -lutil

DPADD+= ${LIBKRB5} ${LIBHX509}
DPADD+= ${LIBASN1} ${LIBCOM_ERR} ${LIBROKEN} ${LIBVERS}
DPADD+= ${LIBHEIMBASE} ${LIBHEIMNTLM} ${LIBWIND}
DPADD+= ${LIBCRYPTO} ${LIBCRYPT}
DPADD+= ${LIBSQLITE3} ${LIBUTIL}

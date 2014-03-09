#	$NetBSD: Makefile,v 1.5 2014/03/09 00:15:46 agc Exp $

PROG=netpgpverify

SRCS= b64.c bignum.c bufgap.c digest.c
SRCS+= libverify.c main.c misc.c
SRCS+= pgpsum.c rsa.c

SRCS+= bzlib.c zlib.c

SRCS+= md5c.c rmd160.c sha1.c sha2.c tiger.c

CPPFLAGS+=-I. ${EXTDIST}/src/netpgpverify

.ifndef PRODUCTION
CPPFLAGS+=-g -O0
LDFLAGS+=-g -O0
.endif

MAN=	netpgpverify.1
WARNS=	6

EXTDIST=${.CURDIR}/../../dist

.PATH: ${EXTDIST}/src/netpgpverify

.include <bsd.prog.mk>

t tst: ${PROG}
	./${PROG} -k ${EXTDIST}/src/netpgpverify/pubring.gpg ${EXTDIST}/src/netpgpverify/NetBSD-6.0_RC1_hashes.asc
	./${PROG} -k ${EXTDIST}/src/netpgpverify/pubring.gpg ${EXTDIST}/src/netpgpverify/NetBSD-6.0_RC1_hashes.gpg
	./${PROG} -v
	./${PROG} -S ${EXTDIST}/src/netpgpverify/sshtest-20140202.pub ${EXTDIST}/src/netpgpverify/data.gpg
	./${PROG} -S ${EXTDIST}/src/netpgpverify/sshtest-20140202.pub ${EXTDIST}/src/netpgpverify/data.sig
	@echo "expected failure, to check bad signatures fail to verify"
	-sed -e 's|A|B|' ${EXTDIST}/src/netpgpverify/data.gpg | ./${PROG} -S ${EXTDIST}/src/netpgpverify/sshtest-20140202.pub 

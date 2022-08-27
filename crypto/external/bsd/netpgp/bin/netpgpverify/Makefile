#	$NetBSD: Makefile,v 1.19 2022/08/27 08:58:31 rillig Exp $

PROG=netpgpverify
BINDIR=		/usr/bin

SRCS= b64.c bignum.c bufgap.c digest.c
SRCS+= libverify.c main.c misc.c
SRCS+= pgpsum.c rsa.c

SRCS+= bzlib.c zlib.c

SRCS+= md5c.c rmd160.c sha1.c sha2.c

CPPFLAGS+=-I${.CURDIR} -I${EXTDIST}/src/netpgpverify
COPTS.libverify.c+= -Wno-format-nonliteral

.ifndef PRODUCTION
CPPFLAGS+=-g -O0
LDFLAGS+=-g -O0
.endif

LINTFLAGS.bzlib.c+=	-X 161	# constant in conditional context
LINTFLAGS.bzlib.c+=	-X 220	# fallthrough on case statement
LINTFLAGS.zlib.c+=	-X 220	# fallthrough on case statement

MAN=	netpgpverify.1
WARNS=	5

EXTDIST=${.CURDIR}/../../dist

.PATH: ${EXTDIST}/src/netpgpverify

.include <bsd.own.mk>

.if defined(HAVE_GCC) && ${HAVE_GCC} >= 7 && ${ACTIVE_CC} == "gcc"
COPTS+=	-Wno-error=implicit-fallthrough
.endif

COPTS.libverify.c+=	${GCC_NO_FORMAT_TRUNCATION}

.include <bsd.prog.mk>

t tst: ${PROG}
	./${PROG} -k ${EXTDIST}/src/netpgpverify/pubring.gpg ${EXTDIST}/src/netpgpverify/NetBSD-6.0_RC1_hashes.asc
	./${PROG} -k ${EXTDIST}/src/netpgpverify/pubring.gpg ${EXTDIST}/src/netpgpverify/NetBSD-6.0_RC1_hashes.gpg
	./${PROG} -v
	./${PROG} -S ${EXTDIST}/src/netpgpverify/sshtest-20140202.pub ${EXTDIST}/src/netpgpverify/data.gpg
	./${PROG} -S ${EXTDIST}/src/netpgpverify/sshtest-20140202.pub ${EXTDIST}/src/netpgpverify/data.sig
	@echo "expected failure, to check bad signatures fail to verify"
	-sed -e 's|A|B|' ${EXTDIST}/src/netpgpverify/data.gpg | ./${PROG} -S ${EXTDIST}/src/netpgpverify/sshtest-20140202.pub 
	@echo "testing signatures with no version"
	./${PROG} -k ${EXTDIST}/src/netpgpverify/pubring.gpg ${EXTDIST}/src/netpgpverify/noversion.asc

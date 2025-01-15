# $NetBSD: Makefile,v 1.6 2025/01/15 15:30:48 christos Exp $

.include <bsd.own.mk>
.include "${.PARSEDIR}/../../Makefile.inc"

DIST=${NETBSDSRCDIR}/external/bsd/pam-u2f/dist

.PATH: ${DIST} ${DIST}/pamu2fcfg

CFFILE=${DIST}/configure.ac

CPPFLAGS+= -I${DIST} -I${.CURDIR}

BINDIR=/usr/bin
PROG=pamu2fcfg
SRCS=pamu2fcfg.c cmdline.c b64.c readpassphrase.c explicit_bzero.c util.c
COPTS.util.c += -Wno-error=stack-protector

LDADD+=-lpam -lfido2 -lcbor -lusbhid -lcrypto -lm
DPADD+=${LIBPAM} ${LIBFIDO2} ${LIBCBOR} ${LIBUSBHID} ${LIBCRYPTO} ${LIBM}

pamu2fcfg.1:
	asciidoc -b docbook45 -d manpage -o pamu2fcfg.1.xml ../../dist/man/pamu2fcfg.1.txt
	xsltproc --nonet --xinclude -o pamu2fcfg.1 'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl' pamu2fcfg.1.xml
	rm -f pamu2fcfg.1.xml

COPTS.util.c+= -Wno-error=deprecated-declarations

.include <bsd.prog.mk>

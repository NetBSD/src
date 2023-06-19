#	$NetBSD: Makefile,v 1.3 2023/06/19 23:11:44 christos Exp $

WARNS?=	2	# XXX -Wextra

NOLINT= # defined

.include <bsd.own.mk>
.include <${.CURDIR}/../../Makefile.inc>

.PATH: ${HEIMDIST}/lib/krb5 ${HEIMDIST}/lib/hcrypto

CPPFLAGS+=-I${HEIMDIST}/lib

PROG= verify_krb5_conf
MAN= verify_krb5_conf.8

SRCS= verify_krb5_conf.c ui.c

#CPPFLAGS+= -I.					\
#	-I${DIST}/heimdal/lib/asn1		\
#	-I${NETBSDSRCDIR}/include/heimdal	\
#	-I${DESTDIR}/usr/include/krb5		\
#	-DHAVE_CONFIG_H

.include <${HEIMBASE}/Makefile.rules.inc>
.include <bsd.prog.mk>

# $NetBSD: Makefile,v 1.4 2000/12/30 17:21:44 sommerfeld Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/krb4/lib/kadm

WARNS?=  1
MKLINT= no

LIB=	kadm

COMPILEET!=	cd ${.CURDIR}/../libcom_err/compile_et && ${PRINTOBJDIR}

kadm_err.c kadm_err.h: kadm_err.et
	${COMPILEET}/compile_et ${DIST}/krb4/lib/kadm/kadm_err.et

ERR_FILES=	kadm_err.c

DPSRCS=		kadm_err.h

SRCS=						\
	kadm_cli_wrap.c				\
	kadm_err.c				\
	kadm_stream.c				\
	kadm_supp.c				\
	check_password.c

INCSDIR= /usr/include/kerberosIV

INCS=	kadm.h					\
	kadm_err.h

LIBKRBINC!=	cd ${.CURDIR}/../libkrb && ${PRINTOBJDIR}

CPPFLAGS+= -I.					\
	-I${DIST}/krb4/lib/kadm			\
	-I${DIST}/krb4/lib/kdb			\
	-I${DIST}/krb4/lib/krb			\
	-I${DIST}/openssl/crypto/des		\
	-I${DIST}/heimdal/lib/com_err		\
	-I${LIBKRBINC}				\
	-I${.CURDIR}/../../include/heimdal	\
	-I${.CURDIR}/../libroken		\
	-I${DIST}/heimdal/lib/roken		\
	-DHAVE_CONFIG_H

CLEANFILES = $(ERR_FILES) kadm_err.h

$(SRCS:.c=.o): kadm_err.h

.include <bsd.lib.mk>

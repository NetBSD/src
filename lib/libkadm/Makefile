# $NetBSD: Makefile,v 1.2 2000/06/16 23:22:58 thorpej Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/krb4/lib/kadm

WARNS?=  1
MKLINT= no

LIB=	kadm

COMPILEET!=	cd ${.CURDIR}/../libcom_err/compile_et && make print-objdir

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

LIBKRBINC!=	cd ${.CURDIR}/../libkrb && make print-objdir

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

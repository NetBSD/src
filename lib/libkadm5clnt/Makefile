# $NetBSD: Makefile,v 1.1.1.1 2000/06/16 19:27:32 thorpej Exp $
DIST=		${.CURDIR}/../../dist
.PATH: ${DIST}/heimdal/lib/kadm5

WARNS?=  1
MKLINT= no

LIB=	kadm5clnt

COMPILEET!=	cd ${.CURDIR}/../libcom_err/compile_et && make print-objdir

kadm5_err.c kadm5_err.h: kadm5_err.et
	${COMPILEET}/compile_et ${DIST}/heimdal/lib/kadm5/kadm5_err.et

ERR_FILES = kadm5_err.c

SRCS	 =		\
	chpass_c.c	\
	common_glue.c	\
	create_c.c	\
	delete_c.c	\
	destroy_c.c	\
	flush_c.c	\
	free.c		\
	get_c.c		\
	get_princs_c.c	\
	init_c.c	\
	kadm5_err.c	\
	marshall.c	\
	modify_c.c	\
	privs_c.c	\
	randkey_c.c	\
	rename_c.c	\
	send_recv.c	\
	$(ERR_FILES)

INCSDIR= /usr/include/kadm5

INCS=	kadm5_err.h admin.h private.h

ASN1!=	cd ${.CURDIR}/../libasn1 && ${MAKE} print-objdir
KRB5!=	cd ${.CURDIR}/../libkrb5 && ${MAKE} print-objdir
HDB!=	cd ${.CURDIR}/../libhdb && ${MAKE} print-objdir

CPPFLAGS+= -I.					\
	 -I${ASN1}				\
	 -I${KRB5}				\
	 -I${DIST}/heimdal/lib/krb5		\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${DIST}/openssl/crypto/rc4		\
	 -I${DIST}/openssl/crypto/des		\
	 -I${HDB}				\
	 -I${DIST}/heimdal/lib/hdb		\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${.CURDIR}/../../include/heimdal	\
	 -I${.CURDIR}/../libroken		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

$(SRCS:.c=.o): kadm5_err.h

CLEANFILES = kadm5_err.h kadm5_err.c

.include <bsd.lib.mk>

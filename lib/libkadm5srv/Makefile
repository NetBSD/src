# $NetBSD: Makefile,v 1.7 2000/12/30 17:21:45 sommerfeld Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/kadm5

WARNS?=  1
MKLINT= no

.include <bsd.own.mk>

LIB=	kadm5srv

COMPILEET!=	cd ${.CURDIR}/../libcom_err/compile_et && ${PRINTOBJDIR}

kadm5_err.c kadm5_err.h: kadm5_err.et
	${COMPILEET}/compile_et ${DIST}/heimdal/lib/kadm5/kadm5_err.et

ERR_FILES = kadm5_err.c

SRCS	 =		\
	acl.c		\
	bump_pw_expire.c \
	chpass_s.c	\
	common_glue.c	\
	context_s.c	\
	create_s.c	\
	delete_s.c	\
	destroy_s.c	\
	ent_setup.c	\
	error.c		\
	flush_s.c	\
	free.c		\
	get_princs_s.c	\
	get_s.c		\
	init_s.c	\
	kadm5_err.c	\
	keys.c		\
	log.c		\
	marshall.c	\
	modify_s.c	\
	privs_s.c	\
	randkey_s.c	\
	rename_s.c	\
	set_keys.c	\
	set_modifier.c	\
	password_quality.c \
	server_glue.c \
	$(ERR_FILES)

ASN1!=	cd ${.CURDIR}/../libasn1 && ${PRINTOBJDIR}
KRB5!=	cd ${.CURDIR}/../libkrb5 && ${PRINTOBJDIR}
HDB!=	cd ${.CURDIR}/../libhdb && ${PRINTOBJDIR}

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

.if defined(NOPIC)
CPPFLAGS+=	-DNO_DLOPEN
.endif

$(SRCS:.c=.o): kadm5_err.h

CLEANFILES = kadm5_err.h kadm5_err.c

.include <bsd.lib.mk>

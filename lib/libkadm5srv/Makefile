# $NetBSD: Makefile,v 1.3 2000/08/02 17:37:57 msaitoh Exp $
DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/kadm5

WARNS?=  1
MKLINT= no

LIB=	kadm5srv

COMPILEET!=	cd ${.CURDIR}/../libcom_err/compile_et && make print-objdir

kadm5_err.c kadm5_err.h: kadm5_err.et
	${COMPILEET}/compile_et ${DIST}/heimdal/lib/kadm5/kadm5_err.et

ERR_FILES = kadm5_err.c

SRCS	 =		\
	acl.c		\
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

.if defined(NOPIC)
CPPFLAGS+=	-DNO_DLOPEN
.endif

$(SRCS:.c=.o): kadm5_err.h

CLEANFILES = kadm5_err.h kadm5_err.c

.include <bsd.lib.mk>

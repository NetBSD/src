# $NetBSD: Makefile,v 1.4 2000/08/03 03:53:38 assar Exp $
DIST=		${.CURDIR}/../../crypto/dist
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

INCS=	kadm5_err.h admin.h kadm5-protos.h private.h kadm5-private.h

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

# Prevent collision with old MIT Kerberos includes -- require manual
# intervention of the operator.
.BEGIN:
.ifmake includes
	@if [ -f ${DESTDIR}${INCSDIR}/kadm_err.h ]; then \
		echo "Error: you must first remove the MIT headers from ${DESTDIR}${INCSDIR}" >&2; \
		false; \
	fi
.endif

.include <bsd.lib.mk>

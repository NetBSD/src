# $NetBSD: Makefile,v 1.21 2006/05/11 23:16:28 mrg Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/heimdal/lib/kadm5

WARNS?=		1

LIB=		kadm5clnt

LIBDPLIBS=	krb5	${.CURDIR}/../libkrb5 \
		hdb	${.CURDIR}/../libhdb \
		roken	${.CURDIR}/../libroken

.if ${USETOOLS} != "yes"
COMPILEETOBJ!=		cd ${NETBSDSRCDIR}/lib/libcom_err/compile_et && ${PRINTOBJDIR}
TOOL_COMPILE_ET=	${COMPILEETOBJ}/compile_et
.endif

kadm5_err.c kadm5_err.h: kadm5_err.et ${TOOL_COMPILE_ET}
	${TOOL_COMPILE_ET} ${DIST}/heimdal/lib/kadm5/kadm5_err.et

ERR_FILES = kadm5_err.c

SRCS	 =		\
	chpass_c.c	\
	common_glue.c	\
	client_glue.c	\
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
	client_glue.c	\
	$(ERR_FILES)

INCS=		kadm5_err.h admin.h kadm5-protos.h private.h kadm5-private.h
INCSDIR=	/usr/include/kadm5

ASN1!=		cd ${NETBSDSRCDIR}/lib/libasn1 && ${PRINTOBJDIR}
KRB5!=		cd ${NETBSDSRCDIR}/lib/libkrb5 && ${PRINTOBJDIR}
HDB!=		cd ${NETBSDSRCDIR}/lib/libhdb && ${PRINTOBJDIR}

CPPFLAGS+= -I.					\
	 -I${ASN1}				\
	 -I${KRB5}				\
	 -I${DIST}/heimdal/lib/krb5		\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${HDB}				\
	 -I${DIST}/heimdal/lib/hdb		\
	 -I${DIST}/heimdal/lib/asn1		\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${NETBSDSRCDIR}/include/heimdal	\
	 -I${NETBSDSRCDIR}/lib/libroken		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

.if (${USE_INET6} != "no")
CPPFLAGS+=-DHAVE_IPV6
.endif

${SRCS:.c=.o}: kadm5_err.h

CLEANFILES=	kadm5_err.h kadm5_err.c

# Prevent collision with old MIT Kerberos includes -- require manual
# intervention of the operator.
.BEGIN:
.ifmake includes
	@if [ -f ${DESTDIR}${INCSDIR}/kadm_err.h ]; then \
		echo "Error: you must first remove the MIT headers from ${DESTDIR}${INCSDIR}" >&2; \
		false; \
	fi
.endif

.if ${HAVE_GCC} == 4
COPTS.marshall.c+=	-Wno-pointer-sign
.endif

.include <bsd.lib.mk>

# $NetBSD: Makefile,v 1.14 2003/07/10 10:34:23 lukem Exp $

NOLINT=		# defined

.include <bsd.own.mk>

DIST=		${NETBSDSRCDIR}/crypto/dist
.PATH:		${DIST}/krb4/lib/kadm

WARNS?=		1

LIB=		kadm

.if ${USETOOLS} != "yes"
COMPILEETOBJ!=		cd ${NETBSDSRCDIR}/lib/libcom_err/compile_et && ${PRINTOBJDIR}
TOOL_COMPILE_ET=	${COMPILEETOBJ}/compile_et
.endif

kadm_err.c kadm_err.h: kadm_err.et ${TOOL_COMPILE_ET}
	${TOOL_COMPILE_ET} ${DIST}/krb4/lib/kadm/kadm_err.et

ERR_FILES=	kadm_err.c
DPSRCS=		kadm_err.h

SRCS=						\
	kadm_cli_wrap.c				\
	kadm_err.c				\
	kadm_stream.c				\
	kadm_supp.c				\
	check_password.c

INCS=		kadm.h kadm_err.h
INCSDIR=	/usr/include/kerberosIV

LIBKRBINC!=	cd ${NETBSDSRCDIR}/lib/libkrb && ${PRINTOBJDIR}

CPPFLAGS+= -I.					\
	-I${DIST}/krb4/lib/kadm			\
	-I${DIST}/krb4/lib/kdb			\
	-I${DIST}/krb4/lib/krb			\
	-I${DIST}/heimdal/lib/com_err		\
	-I${LIBKRBINC}				\
	-I${NETBSDSRCDIR}/include/heimdal	\
	-I${NETBSDSRCDIR}/lib/libroken		\
	-I${DIST}/heimdal/lib/roken		\
	-DHAVE_CONFIG_H

CLEANFILES=	${ERR_FILES} kadm_err.h

${SRCS:.c=.o}: kadm_err.h

.include <bsd.lib.mk>

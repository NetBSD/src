# $NetBSD: Makefile,v 1.10 2001/11/21 05:40:30 gmcgarry Exp $

.include <bsd.own.mk>

DIST=		${.CURDIR}/../../crypto/dist
.PATH: ${DIST}/heimdal/lib/com_err

MKLINT= no

LIB=	com_err
SRCS=	error.c com_err.c roken_rename.h

INCS=	com_err.h com_right.h

MAN=	com_err.3
MLINKS=	com_err.3 com_err_va.3 \
	com_err.3 error_message.3 \
	com_err.3 error_table_name.3 \
	com_err.3 init_error_table.3 \
	com_err.3 set_com_err_hook.3 \
	com_err.3 reset_com_err_hook.3 

.if ${USETOOLS} != "yes"
.PHONY: compile_et

includes: compile_et

compile_et:
	@cd ${.CURDIR}/compile_et && ${MAKE}
.endif

INCSDIR=/usr/include/krb5

CPPFLAGS+= -I.					\
	 -I${.CURDIR}/../../include/heimdal	\
	 -I${.CURDIR}/../libroken		\
	 -I${DIST}/heimdal/lib/com_err		\
	 -I${DIST}/heimdal/lib/roken		\
	 -DHAVE_CONFIG_H

.if ${USETOOLS} != "yes"
SUBDIR=	compile_et
.endif

.include <bsd.lib.mk>
.if ${USETOOLS} != "yes"
.include <bsd.subdir.mk>
.endif

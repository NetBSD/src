#	$NetBSD: Makefile.inc,v 1.16 2002/04/29 04:40:00 sommerfeld Exp $
#
# Makefile for install

PROG=		sysinst
NOMAN=		# defined

.include <bsd.own.mk>		# for mk.conf

DPADD=		${LIBCURSES} ${LIBTERMCAP} ${LIBUTIL}
LDADD=		-lcurses -ltermcap -lutil
LDSTATIC?=	-static

VERDEP=		${NETBSDSRCDIR}/sys/conf/osrelease.sh
VER!=		sh ${VERDEP}

CPPFLAGS+=	-I. -I${.CURDIR}/../.. -I${.CURDIR} \
		-DREL=\"${VER}\" -DMACH=\"${MACHINE}\"

.if defined(SYSINST_FTP_HOST)
CPPFLAGS+=	-DFTP_HOST="${SYSINST_FTP_HOST}"
.endif

.if defined(SYSINST_FTP_DIR)
CPPFLAGS+=	-DFTP_DIR="${SYSINST_FTP_DIR}"
.endif

WARNS=		1

CLEANFILES=	menu_defs.c menu_defs.h menus.def \
		msg_defs.c msg_defs.h msg.def msgtouch

SYSINSTLANG?=	en

.PATH: ${.CURDIR}/../..

MENUC?=		menuc
MSGC?=		msgc

.include <bsd.prog.mk>

menu_defs.c menu_defs.h: menus.def
	${MENUC} menus.def

msg_defs.c msg_defs.h: msg.def
	${MSGC} msg.def

# Needed to get proper dependency checks on osrelease
msgtouch: ${VERDEP}
	touch ${.TARGET}

msg.def: msg.mi.${SYSINSTLANG} msg.md.${SYSINSTLANG} msgtouch
	sed "s/@@VERSION@@/${VER}/" ${.ALLSRC} > ${.TARGET}

menus.def: menus.mi.${SYSINSTLANG} menus.md.${SYSINSTLANG} msgtouch
	sed "s/@@VERSION@@/${VER}/" ${.ALLSRC} > ${.TARGET}

menu_defs.c: msg_defs.h

XOBJS:=${SRCS:M*.c:R:O:S/$/.o/g}

${XOBJS}: msg_defs.h menu_defs.h

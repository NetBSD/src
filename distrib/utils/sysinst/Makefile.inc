#	$NetBSD: Makefile.inc,v 1.20 2003/03/18 20:03:30 fvdl Exp $
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

.if defined(SMALLPROG_INET6)
CPPFLAGS+=-DINET6
.endif


# Host to ftp from.  Default:
#	"ftp.netbsd.org"
#
.if defined(SYSINST_FTP_HOST)
CPPFLAGS+=	-DSYSINST_FTP_HOST=\"${SYSINST_FTP_HOST}\"
.endif

# Top-level ftp directory.  Default:
#	"pub/NetBSD/NetBSD-" + VER + "/" + MACH
#
.if defined(SYSINST_FTP_DIR)
CPPFLAGS+=	-DSYSINST_FTP_DIR=\"${SYSINST_FTP_DIR}\"
.endif

# Top-level CDROM directory.  Default:
#	"/" + MACH
#
.if defined(SYSINST_CDROM_DIR)
CPPFLAGS+=	-DSYSINST_CDROM_DIR=\"${SYSINST_CDROM_DIR}\"
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

#	$NetBSD: Makefile.inc,v 1.10 2000/12/17 09:22:47 jmc Exp $
#
# Makefile for install

.include <bsd.own.mk>		# for mk.conf

PROG=sysinst

LDADD=-lcurses -ltermcap -lutil
LDSTATIC?=-static

.if exists(${.CURDIR}/../../../../sys/conf/osrelease.sh)
VERDEP=${.CURDIR}/../../../../sys/conf/osrelease.sh
VER != sh ${.CURDIR}/../../../../sys/conf/osrelease.sh
.endif

.if exists(${.CURDIR}/../../../../../sys/conf/osrelease.sh)
VERDEP=${.CURDIR}/../../../../../sys/conf/osrelease.sh
VER != sh ${.CURDIR}/../../../../../sys/conf/osrelease.sh
.endif

CPPFLAGS+= -I. -I${.CURDIR}/../.. -I${.CURDIR} \
	 -DREL=\"${VER}\" -DMACH=\"${MACHINE}\"

.if defined(SYSINST_FTP_HOST)
CPPFLAGS+=-DFTP_HOST="${SYSINST_FTP_HOST}"
.endif

.if defined(SYSINST_FTP_DIR)
CPPFLAGS+=-DFTP_DIR="${SYSINST_FTP_DIR}"
.endif

MKMAN=no

WARNS=1

CLEANFILES= menu_defs.c menu_defs.h menus.def msg_defs.c msg_defs.h msg.def msgtouch

SYSINSTLANG?=en

.PATH: ${.CURDIR}/../..

MENUC?=menuc
MSGC?=msgc

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

disks.o install.o main.o net.o upgrade.o util.o: msg_defs.h menu_defs.h

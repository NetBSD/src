#	$NetBSD: Makefile.inc,v 1.29 2003/08/27 10:23:53 dsl Exp $
#
# Makefile for sysinst

PROG=		sysinst
NOMAN=		# defined

SYSINSTLANG?=	en
LANGUAGES?=	en fr pl

MSG_MD?=	msg.md.${SYSINSTLANG}
MENUS_MD?=	menus.md.${SYSINSTLANG}

.include <bsd.own.mk>		# for mk.conf

DPADD=		${LIBCURSES} ${LIBTERMCAP} ${LIBUTIL}
LDADD=		-lcurses -ltermcap -lutil
LDSTATIC?=	-static

UNIF_AWK=	${.CURDIR}/../../unif.awk
MSG_XLAT_SH=	${.CURDIR}/../../msg_xlat.sh

VERDEP=		${NETBSDSRCDIR}/sys/conf/osrelease.sh
VER!=		sh ${VERDEP}

CPPFLAGS+=	-I. -I${.CURDIR}/../.. -I${.CURDIR} \
		-DREL=\"${VER}\" -DMACH=\"${MACHINE}\"

.if defined(SMALLPROG_INET6)
CPPFLAGS+=-DINET6
.endif


# Host to ftp from.  Default:
#	"ftp.NetBSD.org"
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


WARNS=		3

DPSRCS+=	msg_defs.h menu_defs.h

CLEANFILES=	menu_defs.c menu_defs.h menus.def \
		msg_defs.c msg_defs.h msg.def msgtouch \
		sysinstmsgs.*

.PATH: ${.CURDIR}/../..

menu_defs.c menu_defs.h: menus.def
	${TOOL_MENUC} menus.def

msg_defs.c msg_defs.h: msg.def
	${TOOL_MSGC} msg.def

# Needed to get proper dependency checks on osrelease
msgtouch: ${VERDEP}
	touch ${.TARGET}

msg.def: msg.mi.${SYSINSTLANG} ${MSG_MD} msgtouch
	sed "s/@@VERSION@@/${VER}/" ${.ALLSRC} | \
	awk -f ${UNIF_AWK} -v defines="${MD_OPTIONS}" > ${.TARGET}

menus.def: menus.mi ${MENUS_MD} msgtouch
	sed "s/@@VERSION@@/${VER}/" ${.ALLSRC} | \
	awk -f ${UNIF_AWK} -v defines="${MD_OPTIONS}" > ${.TARGET}

.for LANG in ${LANGUAGES:N${SYSINSTLANG}}
sysinstmsgs.${LANG}: msg.mi.${LANG} ${MSG_MD:S/.${SYSINSTLANG}$/.${LANG}/} msgtouch msg_defs.h
	sed "s/@@VERSION@@/${VER}/" ${.ALLSRC:M*.${LANG}} | \
	awk -f ${UNIF_AWK} -v defines="${MD_OPTIONS}" | \
	sh ${MSG_XLAT_SH} > ${.TARGET}

DPSRCS+=	sysinstmsgs.${LANG}
.endfor

.include <bsd.prog.mk>

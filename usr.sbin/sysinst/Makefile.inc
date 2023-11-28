#	$NetBSD: Makefile.inc,v 1.45.2.1 2023/11/28 16:19:51 snj Exp $
#
# Makefile for sysinst

.if exists(${.CURDIR}/../../../Makefile.inc)
.include "${.CURDIR}/../../../Makefile.inc"
.endif

PROG=		sysinst
MAN=		sysinst.8

WARNS=		4

SRCS+=	menu_defs.c msg_defs.c main.c install.c upgrade.c \
	txtwalk.c run.c factor.c net.c disks.c disks_lfs.c util.c geom.c \
	label.c target.c md.c sizemultname.c configmenu.c checkrc.c \
	disklabel.c partitions.c part_edit.c

SRCS+=	${MD_OPTIONS:MAOUT2ELF:S/AOUT2ELF/aout2elf.c/}
SRCS+=	${CPPFLAGS:M-DWSKBD:S/-DWSKBD/wskbd.c/}
SRCS+=	${NODISKLABEL:D:Ubsddisklabel.c}
SRCS+=	${NOPARTMAN:D:Upartman.c}

.PATH: ${.CURDIR}/../../../../sbin/fsck
SRCS+=partutil.c

DPSRCS+= defs.h

SYSINSTLANG?=	en
LANGUAGES?=	de en es fr pl

MENUS_MI=	menus.mi
MENUS_MI+=	${NOPARTMAN:D:Umenus.pm}

.if ${NO_GPT:Uno} != "yes"
CPPFLAGS+=	-DHAVE_GPT
GPT_DIR=${.CURDIR}/../../../../sbin/gpt
SRCS+=	gpt.c gpt_uuid.c
CPPFLAGS.gpt_uuid.c+=	-I${GPT_DIR}
CPPFLAGS.gpt.c+=	-I${GPT_DIR}
.endif
.if ${NO_MBR:Uyes} != "yes"
CPPFLAGS+=	-DHAVE_MBR
SRCS+=	mbr.c
.endif
.if ${NO_CLONES:Uno} == "yes"
CPPFLAGS+=	-DNO_CLONES
.endif

.if ${CHECK_ENTROPY:Uyes} != "no"
MENUS_MI+=	menus.entropy
CPPFLAGS+=	-DCHECK_ENTROPY=1
.endif

MSG_MD?=	msg.md.${SYSINSTLANG}
MENUS_MD?=	menus.md.${SYSINSTLANG}

.include <bsd.own.mk>		# for mk.conf
#.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

DISTRIBVER!=	${HOST_SH} ${NETBSDSRCDIR}/sys/conf/osrelease.sh
DISTRIBVERDEP=	${NETBSDSRCDIR}/sys/sys/param.h \
		${NETBSDSRCDIR}/sys/conf/osrelease.sh

_MKSHTARGET_CREATE?=	${_MKSHMSG_CREATE} ${.CURDIR:T}/${.TARGET}

DPADD=		${LIBCURSES} ${LIBTERMLIB} ${LIBPROP} ${LIBUTIL}
LDADD=		-lcurses -ltermlib -lprop -lutil

UNIF_AWK=	${.CURDIR}/../../unif.awk
MSG_XLAT_SH=	${.CURDIR}/../../msg_xlat.sh
MSG_CMP_SH=	${.CURDIR}/../../msg_cmp.sh

SETS_TAR_SUFF=${"${USE_XZ_SETS:Uno}"!="no":?"tar.xz":"tgz"}

.if ${MKBSDTAR:Uno} == "no"
CPPFLAGS+=	-DUSING_PAXASTAR
.endif

.if ${MKDTB:Uno} != "no"
CPPFLAGS+=	-DHAVE_DTB
.endif

.if ${MKKMOD:Uno} != "no"
CPPFLAGS+=	-DHAVE_MODULES
.endif

CATALOGDIR=	/usr/share/sysinst/catalog
CPPFLAGS+=	-I. -I${.CURDIR}/../.. -I${.CURDIR} \
		-I${.CURDIR}/../../../../sbin/fsck \
		-DSETS_TAR_SUFF=${SETS_TAR_SUFF:Q} \
		-DREL=\"${DISTRIBVER}\" -DMACH=\"${MACHINE}\" \
		-DMACH_${MACHINE} -DARCH_${MACHINE_ARCH} \
		${NODISKLABEL:D-DNO_DISKLABEL} \
		${NOPARTMAN:D-DNO_PARTMAN}

.if ${USE_XZ_SETS:Uno} != "no"
CPPFLAGS+=	-DEXTRACT_NEEDS_BIG_RAM
.endif

.if ${RELEASEMACHINEDIR} != ${MACHINE}
CPPFLAGS+=	-DARCH_SUBDIR="\"${RELEASEMACHINEDIR}\""
CPPFLAGS+=	-DPKG_ARCH_SUBDIR="\"${MACHINE_ARCH}\""
.endif

.if defined(NETBSD_OFFICIAL_RELEASE) && ${NETBSD_OFFICIAL_RELEASE} == "yes"
CPPFLAGS+= -DSYSINST_FTP_HOST=\"ftp.NetBSD.org\" -DNETBSD_OFFICIAL_RELEASE
CPPFLAGS+= -DSYSINST_HTTP_HOST=\"cdn.NetBSD.org\" -DNETBSD_OFFICIAL_RELEASE
CPPFLAGS+= -DREL_PATH=\"netbsd-${DISTRIBVER:C/\.[0-9][_A-Z]*[0-9]*$//:S/./-/}\"
CPPFLAGS+= -DPKG_SUBDIR="\"${DISTRIBVER:C/_.*$//}\""
.else
CPPFLAGS+= -DSYSINST_FTP_HOST=\"nyftp.NetBSD.org\"
CPPFLAGS+= -DSYSINST_HTTP_HOST=\"nycdn.NetBSD.org\"
.if (${DISTRIBVER:M*.99.*})
CPPFLAGS+= -DREL_PATH=\"HEAD\"
CPPFLAGS+= -DPKG_SUBDIR="\"${DISTRIBVER:C/\.99\.[0-9]*[_A-Z]*$//}.0\""
.elif (${DISTRIBVER:M*.[0-9].[0-9]*})
CPPFLAGS+= -DREL_PATH=\"netbsd-${DISTRIBVER:C/\.[0-9][_A-Z]*[0-9]*$//:S/./-/}\"
CPPFLAGS+= -DPKG_SUBDIR="\"${DISTRIBVER:C/_.*$//}\""
.elif (${DISTRIBVER:M*.[0-9]*})
CPPFLAGS+= -DREL_PATH=\"netbsd-${DISTRIBVER:C/\.[0-9][_A-Z]*[0-9]*$//}\"
CPPFLAGS+= -DPKG_SUBDIR="\"${DISTRIBVER:C/_.*$//}\""
.endif
.endif
CPPFLAGS+= -DNETBSD_MAJOR='"${DISTRIBVER:C/\..*//}"'

.if !defined(DEBUG)
CPPFLAGS+=	-DCATALOG_DIR=\"${CATALOGDIR}\"
.endif

.if (defined(DEBUG))
.if ${ACTIVE_CC} == "gcc"
# make gcc run additional passes for better debug info (only needed
# when compiling with -O0)
O0TRACKING=	-fvar-tracking-assignments -fvar-tracking
.endif
CPPFLAGS+=-DDEBUG
DBG +=-g -O0 ${O0TRACKING}
.else
.if defined(SMALLPROG)
COPTS += -Os
.endif
.endif

.if (!defined(SMALLPROG) || defined(SMALLPROG_INET6)) && (${USE_INET6} != "no")
CPPFLAGS+=-DINET6
.endif

COPTS+=	${GCC_NO_FORMAT_TRUNCATION}
COPTS.disklabel.c+= ${GCC_NO_STRINGOP_TRUNCATION}

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


DPSRCS+=	msg_defs.h menu_defs.h

CLEANFILES=	menu_defs.c menu_defs.h menus.def \
		msg_defs.c msg_defs.h msg.def msgtouch \
		sysinstmsgs.*

.PATH: ${.CURDIR}/../..

menu_defs.h: menu_defs.c
	[ -f ${.TARGET} ] || { \
		${_MKSHTARGET_CREATE}; \
		${TOOL_MENUC} menus.def; \
	}
menu_defs.c: menus.def
	${_MKTARGET_CREATE}
	[ ! -f menu_defs.h ] || mv -f menu_defs.h menu_defs.oh
	${TOOL_MENUC} menus.def
	! cmp -s menu_defs.oh menu_defs.h || mv -f menu_defs.oh menu_defs.h
	@rm -f menu_defs.oh

msg_defs.h: msg_defs.c
	[ -f ${.TARGET} ] || { \
		${_MKSHTARGET_CREATE}; \
		${TOOL_MSGC} msg.def; \
	}
msg_defs.c: msg.def
	${_MKTARGET_CREATE}
	[ ! -f msg_defs.h ] || mv -f msg_defs.h msg_defs.oh
	${TOOL_MSGC} msg.def
	! cmp -s msg_defs.oh msg_defs.h || mv -f msg_defs.oh msg_defs.h
	@rm -f msg_defs.oh

# Needed to get proper dependency checks on osrelease
msgtouch: ${DISTRIBVERDEP}
	touch ${.TARGET}

.if !defined(NOPARTMAN)
MSG_MD+=	msg.pm.${SYSINSTLANG}
.endif

.if ${CHECK_ENTROPY:Uyes} != "no"
MSG_MD+=	msg.entropy.${SYSINSTLANG}
.endif

msg.def: msg.mi.${SYSINSTLANG} ${MSG_MD} msgtouch
	${_MKTARGET_CREATE}
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" ${.ALLSRC} | \
	${TOOL_SED} "s/@@MACHINE@@/${MACHINE}/" | \
	${TOOL_AWK} -f ${UNIF_AWK} -v defines="${MD_OPTIONS}" > ${.TARGET}

menus.def: ${MENUS_MI} ${MENUS_MD} msgtouch
	${_MKTARGET_CREATE}
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" ${.ALLSRC} | \
	${TOOL_SED} "s/@@MACHINE@@/${MACHINE}/" | \
	${TOOL_AWK} -f ${UNIF_AWK} -v defines="${MD_OPTIONS}" > ${.TARGET}

sysinstmsgs.fmtcnt: msg.mi.${SYSINSTLANG} ${MSG_MD} msgtouch msg_defs.h
	${_MKTARGET_CREATE}
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" \
		${.ALLSRC:M*.${SYSINSTLANG}} | \
	${TOOL_SED} "s/@@MACHINE@@/${MACHINE}/" | \
	${TOOL_AWK} -f ${UNIF_AWK} -v defines="${MD_OPTIONS}" | \
	${HOST_SH} ${MSG_XLAT_SH} -c > ${.TARGET}

.for LANG in ${LANGUAGES:N${SYSINSTLANG}}
sysinstmsgs.${LANG}: msg.mi.${LANG} ${MSG_MD:S/.${SYSINSTLANG}$/.${LANG}/} msgtouch msg_defs.h sysinstmsgs.fmtcnt
	${_MKTARGET_CREATE}
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" ${.ALLSRC:M*.${LANG}} | \
	${TOOL_SED} "s/@@MACHINE@@/${MACHINE}/" | \
	${TOOL_AWK} -f ${UNIF_AWK} -v defines="${MD_OPTIONS}" | \
	${HOST_SH} ${MSG_XLAT_SH} -f sysinstmsgs.fmtcnt > ${.TARGET}

DPSRCS+=	sysinstmsgs.${LANG}
FILES+=		sysinstmsgs.${LANG}
FILESDIR_sysinstmsgs.${LANG}=	${CATALOGDIR}
.endfor

.PHONY:	check-lang
check-lang:	msg.def msg_defs.h
	${_MKTARGET_CREATE}
	${TOOL_SED} "s/@@VERSION@@/${DISTRIBVER}/" msg.def | \
	${TOOL_SED} "s/@@MACHINE@@/${MACHINE}/" | \
	${TOOL_AWK} -f ${UNIF_AWK} -v defines="${MD_OPTIONS}" | \
	${HOST_SH} ${MSG_XLAT_SH} -i > /tmp/sysinst.en
	${HOST_SH} ${MSG_CMP_SH} msg_defs.h /tmp/sysinst.en sysinstmsgs.${LANG}
	@rm /tmp/sysinst.en

COPTS.util.c+=	${GCC_NO_STRINGOP_TRUNCATION}
COPTS+=		-fcommon

.include <bsd.prog.mk>

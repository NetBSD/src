#	$NetBSD: Makefile,v 1.4 2011/08/22 18:54:06 jym Exp $

.include <bsd.own.mk>

CONFIGFILES=	daily.conf monthly.conf security.conf weekly.conf
FILESDIR=	/etc/defaults
FILESMODE=	${NONBINMODE}

# Manage arch-specific rc.conf(5) file
CONFIGFILES+=	rc_conf
FILESNAME_rc_conf=	rc.conf
FILESBUILD_rc_conf=	yes

RCCONF_SRCS=	${.CURDIR}/rc.conf
.if exists(../etc.${MACHINE}/rc.conf)
RCCONF_SRCS+=	${.CURDIR}/../etc.${MACHINE}/rc.conf
.endif

rc_conf: ${RCCONF_SRCS}
	${_MKTARGET_CREATE}
	${TOOL_CAT} ${RCCONF_SRCS} > ${.TARGET}

.include <bsd.prog.mk>

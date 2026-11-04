#	$NetBSD: Makefile,v 1.7.12.1 2026/11/04 22:37:16 perseant Exp $

.include <bsd.own.mk>

CONFIGFILES=	daily.conf monthly.conf pkgpath.conf \
		    security.conf weekly.conf

.if (${MKNPF} != "no")
CONFIGFILES+=	npf.boot.conf
.endif

FILESDIR=	/etc/defaults
FILESMODE=	${NONBINMODE}

# Manage arch-specific rc.conf(5) file
CONFIGFILES+=	rc_conf
FILESNAME_rc_conf=	rc.conf
FILESBUILD_rc_conf=	yes

# If present, rc.conf.append is added to the end of the default rc.conf file
RCCONF_SRCS=	${.CURDIR}/rc.conf
.if exists(../etc.${MACHINE}/rc.conf.append)
RCCONF_SRCS+=	${.CURDIR}/../etc.${MACHINE}/rc.conf.append
.endif

rc_conf: ${RCCONF_SRCS}
	${_MKTARGET_CREATE}
	${TOOL_CAT} ${RCCONF_SRCS} > ${.TARGET}

.include <bsd.prog.mk>

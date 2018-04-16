#	$NetBSD: Makefile,v 1.16.2.1 2018/04/16 01:57:31 pgoyette Exp $

.include <bsd.own.mk>
.include "${NETBSDSRCDIR}/distrib/common/Makefile.distrib"

IMAGE=		sshramdisk.fs
IMAGESIZE=	28000k
MAKEFS_FLAGS+=	-f 15

.if ${MKREPRO_TIMESTAMP:Uno} != "no"
MAKEFS_TIMESTAMP=-T "${MKREPRO_TIMESTAMP}"
PAX_TIMESTAMP=--timestamp "${MKREPRO_TIMESTAMP}"
.endif

WARNS=		1
DBG=		-Os

CRUNCHBIN=	ramdiskbin
LISTS=		${.CURDIR}/list ${DISTRIBDIR}/common/list.sysinst
MTREECONF=	${.CURDIR}/mtree.conf
.if empty(MACHINE_ARCH:M*eb)
IMAGEENDIAN=	le
.else
IMAGEENDIAN=	be
.endif
MAKEDEVTARGETS=	ramdisk
IMAGEDEPENDS=	${CRUNCHBIN} \
		dot.profile master.passwd sshd_config ssh_host_dsa_key \
		${NETBSDSRCDIR}/etc/group \
		${NETBSDSRCDIR}/etc/netconfig ${DISTRIBDIR}/common/protocols \
		${DISTRIBDIR}/common/services
IMAGEPREBUILD=	${TOOL_PAX} ${PAX_TIMESTAMP} -rw -pp ${DESTDIR}/libdata/firmware ${WORKDIR}

# Use stubs to eliminate some large stuff from libc
HACKSRC=	${DISTRIBDIR}/utils/libhack
.include	"${HACKSRC}/Makefile.inc"
${CRUNCHBIN}:	libhack.o

USE_PAM := no
USE_YP := no
USE_SKEY := no
USE_KERBEROS := no
USE_LDAP := no
.export USE_PAM
.export USE_YP
.export USE_SKEY
.export USE_KERBEROS
.export USE_LDAP

.include "${DISTRIBDIR}/common/Makefile.crunch"
.include "${DISTRIBDIR}/common/Makefile.dhcpcd"
.include "${DISTRIBDIR}/common/Makefile.makedev"
.include "${DISTRIBDIR}/common/Makefile.image"

release:

.include <bsd.prog.mk>

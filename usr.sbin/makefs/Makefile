#	$NetBSD: Makefile,v 1.19 2006/05/12 01:18:25 mrg Exp $
#

.include <bsd.own.mk>

PROG=	makefs
SRCS=	buf.c \
	cd9660.c cd9660_strings.c cd9660_debug.c cd9660_eltorito.c \
	cd9660_write.c cd9660_conversion.c iso9660_rrip.c \
	ffs.c ffs_alloc.c ffs_balloc.c ffs_bswap.c ffs_subr.c ffs_tables.c \
	getid.c \
	makefs.c misc.c mkfs.c \
	pack_dev.c \
	spec.c stat_flags.c \
	ufs_bmap.c \
	walk.c
MAN=	makefs.8

LSSRC=		${NETBSDSRCDIR}/bin/ls
MKNODSRC=	${NETBSDSRCDIR}/sbin/mknod
MTREESRC=	${NETBSDSRCDIR}/usr.sbin/mtree
UFSSRC=		${NETBSDSRCDIR}/sys/ufs
CD9660SRC=	${NETBSDSRCDIR}/sys/fs/cd9660

CPPFLAGS+=	-I${.CURDIR} \
		-I${LSSRC} -I${MKNODSRC} -I${MTREESRC} -I${CD9660SRC}
.PATH:		${.CURDIR}/ffs ${UFSSRC}/ffs \
		${.CURDIR}/cd9660 ${CD9660SRC} \
		${LSSRC} ${MKNODSRC} ${MTREESRC}

WARNS?=	3

.if !defined(HOSTPROGNAME)
.if ${HAVE_GCC} == 4
.for f in cd9660_debug cd9660_write
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif
.endif

.include <bsd.prog.mk>

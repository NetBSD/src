#	$NetBSD: Makefile,v 1.23 2009/01/10 22:06:29 bjh21 Exp $
#

.include <bsd.own.mk>

PROG=	makefs
SRCS=	buf.c \
	cd9660.c cd9660_strings.c cd9660_debug.c cd9660_eltorito.c \
	cd9660_write.c cd9660_conversion.c iso9660_rrip.c cd9660_archimedes.c \
	ffs.c ffs_alloc.c ffs_balloc.c ffs_bswap.c ffs_subr.c ffs_tables.c \
	getid.c \
	makefs.c misc.c mkfs.c \
	pack_dev.c \
	spec.c \
	ufs_bmap.c \
	walk.c
MAN=	makefs.8

MKNODSRC=	${NETBSDSRCDIR}/sbin/mknod
MTREESRC=	${NETBSDSRCDIR}/usr.sbin/mtree
UFSSRC=		${NETBSDSRCDIR}/sys/ufs
CD9660SRC=	${NETBSDSRCDIR}/sys/fs/cd9660

CPPFLAGS+=	-I${.CURDIR} \
		-I${MKNODSRC} -I${MTREESRC} -I${CD9660SRC}
.PATH:		${.CURDIR}/ffs ${UFSSRC}/ffs \
		${.CURDIR}/cd9660 ${CD9660SRC} \
		${MKNODSRC} ${MTREESRC}

.if (${HOSTPROG:U} == "")
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

WARNS?=	3

.if !defined(HOSTPROGNAME)
.if (defined(HAVE_GCC) && ${HAVE_GCC} == 4) || defined(HAVE_PCC)
.for f in cd9660_debug cd9660_write
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif
.endif

.include <bsd.prog.mk>

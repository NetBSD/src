#	$NetBSD: Makefile,v 1.42 2025/09/24 01:29:10 mrg Exp $
#

.include <bsd.own.mk>

PROG=	makefs
SRCS=	cd9660.c chfs.c ffs.c v7fs.c msdos.c udf.c\
	getid.c \
	makefs.c misc.c \
	pack_dev.c \
	spec.c \
	walk.c
MAN=	makefs.8

MKNODSRC=	${NETBSDSRCDIR}/sbin/mknod
MTREESRC=	${NETBSDSRCDIR}/usr.sbin/mtree

CPPFLAGS+=	-I${.CURDIR} -I${MKNODSRC} -I${MTREESRC} -DMAKEFS
#CPPFLAGS+=	-DMSDOSFS_DEBUG
.PATH:		${MKNODSRC} ${MTREESRC}

.include "${.CURDIR}/cd9660/Makefile.inc"
.include "${.CURDIR}/chfs/Makefile.inc"
.include "${.CURDIR}/ffs/Makefile.inc"
.include "${.CURDIR}/v7fs/Makefile.inc"
.include "${.CURDIR}/msdos/Makefile.inc"
.include "${.CURDIR}/udf/Makefile.inc"

.if defined(HOSTPROG)
LDADD+=	-lm
.else
DPADD+= ${LIBUTIL} ${LIBM}
LDADD+= -lutil -lm

COPTS.cd9660.c+=		${CC_WNO_STRINGOP_TRUNCATION} ${CC_WNO_FORMAT_TRUNCATION}
COPTS.cd9660_conversion.c+=	${CC_WNO_FORMAT_TRUNCATION}
COPTS.udf.c+=			${CC_WNO_STRINGOP_TRUNCATION}
COPTS.v7fs_estimate.c+=		${CC_WNO_STRINGOP_TRUNCATION}
COPTS.v7fs_populate.c+=		${CC_WNO_STRINGOP_TRUNCATION} ${CC_WNO_FORMAT_TRUNCATION}
.endif

# XXX GCC 14
.if ${MACHINE} == "vax"
COPTS.msdosfs_vfsops.c+=	-O0
.endif

.include <bsd.prog.mk>
# DO NOT DELETE

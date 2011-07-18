#	$NetBSD: Makefile,v 1.29 2011/07/18 22:52:37 tron Exp $
#

WARNS?=	3	# XXX -Wsign-compare

.include <bsd.own.mk>

PROG=	makefs
SRCS=	cd9660.c ffs.c v7fs.c \
	getid.c \
	makefs.c misc.c \
	pack_dev.c \
	spec.c \
	walk.c
MAN=	makefs.8

MKNODSRC=	${NETBSDSRCDIR}/sbin/mknod
MTREESRC=	${NETBSDSRCDIR}/usr.sbin/mtree

CPPFLAGS+=	-I${.CURDIR} -I${MKNODSRC} -I${MTREESRC}
.PATH:		${MKNODSRC} ${MTREESRC}

.include "${.CURDIR}/cd9660/Makefile.inc"
.include "${.CURDIR}/ffs/Makefile.inc"
.include "${.CURDIR}/v7fs/Makefile.inc"

.if !defined(HOSTPROG)
DPADD+= ${LIBUTIL}
LDADD+= -lutil
.endif

.include <bsd.prog.mk>

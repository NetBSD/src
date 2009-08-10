#	$NetBSD: Makefile,v 1.1 2009/08/10 07:16:06 haad Exp $

.include "../Makefile.inc"

CFLAGS+= -g -fno-inline

KMOD=	solaris

ZFSDIR=	${S}/../external/cddl/osnet

.PATH:	${ZFSDIR}/dist/common/avl

SRCS=	avl.c

.PATH:	${ZFSDIR}/dist/common/nvpair

SRCS+=	nvpair.c
SRCS+=	nvpair_alloc_fixed.c

.PATH:	${ZFSDIR}/dist/common/unicode

SRCS+=	u8_textprep.c

.PATH:	${ZFSDIR}/dist/uts/common/os

SRCS+=	fm.c
SRCS+=	list.c
SRCS+=	nvpair_alloc_system.c

.PATH:	${ZFSDIR}/dist/uts/common/rpc

SRCS+=	xdr.c
SRCS+=	xdr_array.c
SRCS+=	xdr_mem.c

.PATH:	${ZFSDIR}/dist/uts/common/zmod

SRCS+=	adler32.c
SRCS+=	crc32.c
SRCS+=	deflate.c
SRCS+=	inffast.c
SRCS+=	inflate.c
SRCS+=	inftrees.c
SRCS+=	trees.c
SRCS+=	zmod.c
SRCS+=	zmod_subr.c
SRCS+=	zutil.c

.PATH:  ${ZFSDIR}/dist/common/acl

SRCS+= acl_common.c

.PATH:	${ZFSDIR}/sys/kern

SRCS+=	kobj.c
SRCS+=	kstat.c
SRCS+=	misc.c
SRCS+=	policy.c
SRCS+=	string.c
SRCS+=	zone.c
SRCS+=	callb.c
SRCS+=	ddi.c
SRCS+=	mod.c
SRCS+=	printf.c
SRCS+=	taskq.c
SRCS+=	vfs.c

WARNS=		0
NOGCCERROR=	yes

CWARNFLAGS+=	-Wall
CWARNFLAGS+=	-Wno-unknown-pragmas
CWARNFLAGS+=	-Wno-missing-braces
CWARNFLAGS+=	-Wno-parentheses
CWARNFLAGS+=	-Wno-uninitialized
CWARNFLAGS+=	-Wno-unused
CWARNFLAGS+=	-Wno-switch
CWARNFLAGS+=	-Wno-strict-prototypes
CWARNFLAGS+=	-Wno-missing-prototypes
CWARNFLAGS+=	-Wno-format

CPPFLAGS+=	-I${.CURDIR}/../..
CPPFLAGS+=	-I${.CURDIR}/../../../../include
CPPFLAGS+=	-I${ZFSDIR}/sys
CPPFLAGS+=	-I${ZFSDIR}/dist/common/acl
CPPFLAGS+=	-I${ZFSDIR}/dist/uts/common/zmod
CPPFLAGS+=	-I${ZFSDIR}/dist/uts/common
CPPFLAGS+=      -I${ZFSDIR}/sys/sys

CPPFLAGS+=	-Dcaddr_t=__caddr_t "-D__va_list=va_list"
CPPFLAGS+=	-std=c99
# CPPFLAGS+=	-D_NFS_NFS_H_
# CPPFLAGS+=	-D_PROPLIB_ZFS_CONFLICT

CFLAGS+=	-g -O0 -DDIAGNOSTIC
.include <bsd.kmodule.mk>

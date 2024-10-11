# $NetBSD: Makefile,v 1.22.2.1 2024/10/11 08:54:39 martin Exp $

RUMPPRG=cgdconfig
MAN=	cgdconfig.8

.include <bsd.own.mk>

SRCS+=	cgdconfig.c		\
	cgdlex.l		\
	cgdparse.y		\
	hkdf_hmac_sha256.c	\
	pkcs5_pbkdf2.c		\
	params.c		\
	utils.c

CPPFLAGS+= -I${.CURDIR} -I. -DYY_NO_INPUT

YHEADER=1

DPADD=  ${LIBUTIL} ${LIBCRYPT} ${LIBY} ${LIBL}
LDADD=  -lutil -lcrypt -ly -ll

.if ${MKARGON2} != "no"
SRCS+=	argon2_utils.c
CPPFLAGS+=	-DHAVE_ARGON2
.if defined(CRUNCHEDPROG)
ARGON2_NO_THREADS=1
.endif
.include "${NETBSDSRCDIR}/external/apache2/argon2/lib/libargon2/Makefile.inc"
.endif

.if ${MKZFS} != "no"
CPPFLAGS+=	-DHAVE_ZFS

OSNET=${NETBSDSRCDIR}/external/cddl/osnet
CPPFLAGS.cgdconfig.c+=	-I${OSNET}/include
CPPFLAGS.cgdconfig.c+=	-I${OSNET}/sys
CPPFLAGS.cgdconfig.c+=	-I${OSNET}/dist/head
CPPFLAGS.cgdconfig.c+=	-I${OSNET}/dist/lib/libzpool/common
CPPFLAGS.cgdconfig.c+=	-I${OSNET}/dist/uts/common
CPPFLAGS.cgdconfig.c+=	-I${OSNET}/dist/uts/common/fs/zfs

COPTS.cgdconfig.c+=	-Wno-unknown-pragmas
COPTS.cgdconfig.c+=	-Wno-strict-prototypes
.endif

.include <bsd.prog.mk>

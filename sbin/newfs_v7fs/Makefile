# $NetBSD: Makefile,v 1.2 2011/07/01 01:26:15 mrg Exp $

.include <bsd.own.mk>

V7FS = ${NETBSDSRCDIR}/sys/fs/v7fs
PROG=	newfs_v7fs
MAN=	newfs_v7fs.8
SRCS= newfs_v7fs.c main.c v7fs_endian.c v7fs_superblock.c v7fs_inode.c	\
v7fs_datablock.c v7fs_dirent.c v7fs_io.c v7fs_io_user.c progress.c

# use progress meter.
FSCK=	${NETBSDSRCDIR}/sbin/fsck

DPADD+= ${LIBUTIL}
LDADD+= -lutil
CPPFLAGS+=-DV7FS_EI -I${V7FS} -I${FSCK} -g
.PATH: ${V7FS} ${FSCK}

.if defined(HAVE_GCC)
COPTS.newfs_v7fs.c+=	-Wno-pointer-sign
.endif

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.14.16.1 2011/06/23 14:18:42 cherry Exp $
#	@(#)Makefile	8.1 (Berkeley) 6/5/93

.include <bsd.own.mk>

PROG=	fsck_ext2fs
MAN=    fsck_ext2fs.8
SRCS=	dir.c inode.c main.c pass1.c pass1b.c pass2.c pass3.c pass4.c \
	pass5.c fsutil.c setup.c utilities.c ext2fs_bswap.c
FSCK=	${NETBSDSRCDIR}/sbin/fsck
CPPFLAGS+= -I${FSCK}
.PATH:	${NETBSDSRCDIR}/sys/ufs/ext2fs ${FSCK}

.include <bsd.prog.mk>

LDADD+=-lutil
DPADD+=${LIBUTIL}

.if defined(HAVE_GCC) || defined(HAVE_PCC)
.for f in pass5 utilities
COPTS.${f}.c+=  -Wno-pointer-sign
.endfor
.endif

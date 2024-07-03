#	$NetBSD: Makefile,v 1.3.4.2 2024/07/03 18:38:54 martin Exp $
#	$OpenBSD: Makefile,v 1.8 2022/07/11 03:11:49 daniel Exp $

# NOTE: The mkhybrid distrib also makes an "mkisofs" which is the same binary
# linked. When we know mkhybrid makes distrib CDs ok we should modify this
# to make mkisofs as a link to this, rather than the (earlier) version in
# the tree.

DIST=	${NETBSDSRCDIR}/external/gpl2/mkhybrid/dist

PROG=	mkhybrid
MAN=	mkhybrid.8
BINDIR=	/usr/sbin

.PATH:	${DIST}
SRCS=	mkisofs.c tree.c write.c hash.c rock.c multi.c \
	joliet.c match.c name.c eltorito.c \
	apple.c volume.c desktop.c mac_label.c		
SRCS+=	getopt.c getopt1.c

.PATH:	${DIST}/libhfs_iso
SRCS+=	block.c btree.c data.c file.c hfs.c low.c lvolume.c node.c record.c

.PATH:	${DIST}/libfile
SRCS+=	apprentice.c lfile.c softmagic.c

CPPFLAGS+=-DSYSTEM_ID_DEFAULT=\"NetBSD\" -DAPPLE_HYB -DVANILLA_AUTOCONF \
	-DHAVE_CONFIG_H \
	-I${DIST} -I${DIST}/include \
	-I${DIST}/libhfs_iso \
	-I${DIST}/libfile

.include <bsd.prog.mk>

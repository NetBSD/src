#	$NetBSD: Makefile,v 1.18 1997/05/30 01:55:11 cjs Exp $

LIB=	sa
NOPIC=
NOPROFILE=

# this library needed only during build
NOINSTALL=1

SA_USE_CREAD?= no
SA_INCLUDE_NET?= yes

#DEBUGCPPFLAGS= -DBOOTP_DEBUG -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG -DRPC_DEBUG -DRARP_DEBUG -DDEBUG -DPARANOID
CPPFLAGS=	-I. ${SACPPFLAGS} ${SAMISCCPPFLAGS} -DSTANDALONE \
		-DCOMPAT_UFS ${DEBUGCPPFLAGS}

#CFLAGS+= -ansi -pedantic -Wall

# stand routines
SRCS+=	alloc.c bcopy.c exit.c exec.c getfile.c gets.c globals.c \
	memcmp.c memcpy.c printf.c strerror.c

# io routines
SRCS+=	closeall.c dev.c disklabel.c dkcksum.c ioctl.c nullfs.c stat.c fstat.c
SRCS+=	close.c lseek.c open.c read.c write.c
.if (${SA_USE_CREAD} == "yes")
CPPFLAGS+= -D__INTERNAL_LIBSA_CREAD
SRCS+=	cread.c
.endif

.if (${SA_INCLUDE_NET} == "yes")
# network routines
SRCS+=	arp.c ether.c in_cksum.c net.c netif.c rpc.c
.endif

# network info services:
SRCS+=	bootp.c rarp.c bootparam.c

# boot filesystems
SRCS+=	ufs.c nfs.c cd9660.c

.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`

#	$NetBSD: Makefile,v 1.43.2.2 2001/09/21 22:36:31 nathanw Exp $

LIB=	sa
MKPIC=	no
MKPROFILE=no

SA_USE_CREAD?= no		# Read compressed kernels
SA_INCLUDE_NET?= yes		# Netboot via TFTP, NFS
SA_USE_LOADFILE?= no		#

#DEBUGCPPFLAGS= -DBOOTP_DEBUG -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG -DRPC_DEBUG -DRARP_DEBUG -DNET_DEBUG -DDEBUG -DPARANOID
CPPFLAGS=	-I${SADIR} ${SACPPFLAGS} ${SAMISCCPPFLAGS} \
		-DCOMPAT_UFS ${DEBUGCPPFLAGS}

#COPTS+= -ansi -pedantic -Wall

.PATH.c: ${SADIR}

# stand routines
SRCS+=	alloc.c bcmp.c bcopy.c bzero.c errno.c exit.c exec.c getfile.c gets.c \
	globals.c memcmp.c memcpy.c memmove.c memset.c panic.c printf.c \
	snprintf.c sprintf.c strerror.c subr_prf.c twiddle.c vsprintf.c \
	checkpasswd.c

# io routines
SRCS+=	closeall.c dev.c disklabel.c dkcksum.c ioctl.c nullfs.c stat.c fstat.c
SRCS+=	close.c lseek.c open.c read.c write.c
.if (${SA_USE_CREAD} == "yes")
CPPFLAGS+= -D__INTERNAL_LIBSA_CREAD
SRCS+=	cread.c
.endif

.if (${SA_USE_LOADFILE} == "yes")
SRCS+=	loadfile.c
.endif
.if (${SA_INCLUDE_NET} == "yes")
# network routines
SRCS+=	arp.c ether.c in_cksum.c net.c netif.c rpc.c udp.c

# network info services:
SRCS+=	bootp.c rarp.c bootparam.c

# boot filesystems
SRCS+=	nfs.c tftp.c
.endif
SRCS+=	lfs.c ufs.c ufs_ls.c cd9660.c ustarfs.c dosfs.c

# only needed during build
libinstall::

.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`

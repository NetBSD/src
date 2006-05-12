#	$NetBSD: Makefile,v 1.59 2006/05/12 03:29:11 mrg Exp $

LIB=	sa
NOPIC=	# defined
NOPROFILE=# defined

SA_USE_CREAD?= no		# Read compressed kernels
SA_INCLUDE_NET?= yes		# Netboot via TFTP, NFS
SA_USE_LOADFILE?= no		# Generic executable loading support

#DEBUGCPPFLAGS= -DBOOTP_DEBUG -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG -DRPC_DEBUG -DRARP_DEBUG -DNET_DEBUG -DDEBUG -DPARANOID
CPPFLAGS=	-I${SADIR} ${SACPPFLAGS} ${SAMISCCPPFLAGS} \
		-DCOMPAT_UFS ${DEBUGCPPFLAGS}

#COPTS+= -ansi -pedantic -Wall

.PATH.c: ${SADIR}

# stand routines
SRCS+=	alloc.c bcopy.c bzero.c errno.c exit.c exec.c files.c \
	getfile.c gets.c globals.c memcmp.c memcpy.c memmove.c memset.c \
	panic.c printf.c qsort.c snprintf.c sprintf.c strerror.c subr_prf.c \
	twiddle.c vsprintf.c checkpasswd.c

# io routines
SRCS+=	closeall.c dev.c disklabel.c dkcksum.c ioctl.c nullfs.c stat.c fstat.c
SRCS+=	close.c lseek.c open.c read.c write.c
.if (${SA_USE_CREAD} == "yes")
CPPFLAGS+= -D__INTERNAL_LIBSA_CREAD
SRCS+=	cread.c
.endif

.if (${SA_USE_LOADFILE} == "yes")
SRCS+=	loadfile.c loadfile_aout.c loadfile_ecoff.c loadfile_elf32.c \
	loadfile_elf64.c
.endif

.if (${SA_INCLUDE_NET} == "yes")
# network routines
SRCS+=	arp.c ether.c ether_sprintf.c ip_cksum.c net.c netif.c rpc.c udp.c

# network info services:
SRCS+=	bootp.c rarp.c bootparam.c

# boot filesystems
SRCS+=	nfs.c tftp.c
.endif

SRCS+=	lfsv1.c lfsv2.c ffsv1.c ffsv2.c ufs_ls.c cd9660.c ustarfs.c dosfs.c
# for historic comparibility ufs == ffsv1
SRCS+=	ufs.c

# only needed during build
libinstall::

.undef DESTDIR
.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`

.if ${HAVE_GCC} == 4
CPPFLAGS+=	-Wno-pointer-sign
.endif

#	$NetBSD: Makefile,v 1.39 1999/11/12 01:26:28 simonb Exp $

LIB=	sa
MKPIC=	no
MKPROFILE=no

SA_USE_CREAD?= no
SA_INCLUDE_NET?= yes
SA_USE_LOADFILE?= no

#DEBUGCPPFLAGS= -DBOOTP_DEBUG -DNETIF_DEBUG -DETHER_DEBUG -DNFS_DEBUG -DRPC_DEBUG -DRARP_DEBUG -DDEBUG -DPARANOID
CPPFLAGS=	-I${SADIR} ${SACPPFLAGS} ${SAMISCCPPFLAGS} \
		-DCOMPAT_UFS ${DEBUGCPPFLAGS}

#COPTS+= -ansi -pedantic -Wall

.PATH.c: ${SADIR}

# stand routines
SRCS+=	alloc.c bcmp.c bcopy.c bzero.c errno.c exit.c exec.c getfile.c gets.c \
	globals.c htonl.c htons.c inet_addr.c intoa.c memcmp.c memcpy.c \
	memset.c ntohs.c ntohl.c panic.c printf.c snprintf.c sprintf.c \
	strlen.c strcmp.c strncmp.c strcpy.c strncpy.c strcat.c strerror.c \
	subr_prf.c twiddle.c vsprintf.c checkpasswd.c

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
SRCS+=	arp.c ether.c in_cksum.c net.c netif.c rpc.c

# network info services:
SRCS+=	bootp.c rarp.c bootparam.c

# boot filesystems
SRCS+=	nfs.c tftp.c
.endif
SRCS+=	lfs.c ufs.c ufs_ls.c cd9660.c ustarfs.c

# quad routines
SRCS+=	ashrdi3.c

# work-around for gcc annoyance
SRCS+=	__main.c

# only needed during build
libinstall::

.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`

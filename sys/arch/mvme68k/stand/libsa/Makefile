#	$NetBSD: Makefile,v 1.28 2005/12/27 12:38:53 he Exp $

LIB=sa

# Logically src/sys
DIR_SA=$S/lib/libsa
DIR_KERN=$S/lib/libkern
DIR_LIBC=$S/../common/lib/libc

SRC_net= arp.c \
	 bootp.c bootparam.c \
	 dev_net.c \
	 ether.c \
	 in_cksum.c \
	 net.c netif.c nfs.c \
	 rarp.c rpc.c

SRC_sa= alloc.c \
	close.c closeall.c \
	dev.c dev_net.c dkcksum.c \
	ether_sprintf.c exit.c \
	files.c fstat.c \
	getfile.c gets.c globals.c \
	intoa.c \
	loadfile.c loadfile_aout.c loadfile_elf32.c lseek.c \
	memmove.c memcmp.c memcpy.c memset.c \
	nullfs.c \
	open.c \
	panic.c printf.c \
	read.c \
	snprintf.c sprintf.c strerror.c subr_prf.c \
	twiddle.c \
	udp.c ufs.c

SRC_common+= ashrdi3.c inet_addr.c strncpy.c strlen.c

# XXX Needed until libsa's memcmp.c doesn't depend on bcmp...
SRC_sa+= bcmp.c

SRC_libc_inet= inet_addr.c
SRC_libc_quad= ashldi3.c ashrdi3.c
SRC_libc_string= strcmp.c strlen.c strncpy.c

SRC_mvme= exec_mvme.c

SRC_here= bugdev.c \
	  chiptotime.c clock.c \
	  parse_args.c

SRCS= ${SRC_net} ${SRC_sa} ${SRC_kern} ${SRC_common} ${SRC_mvme} ${SRC_here}

# DBG= -DDEBUG -DNETIF_DEBUG -DNFS_DEBUG -DRPC_DEBUG \
# 	-DNET_DEBUG -DRARP_DEBUG -DETHER_DEBUG

DEFS= ${DBG} #-fno-defer-pop

CPPFLAGS+= -DSUPPORT_BOOTP -DSUPPORT_DHCP
CPPFLAGS+= -DSUPPORT_RARP -DSUPPORT_BOOTPARAM
CPPFLAGS+= -DSUPPORT_NFS -DNFS_NOSYMLINK

CLEANFILES+= SRT0.o

.include "../Makefile.booters"
.include "${S}/../common/lib/libc/Makefile.inc"

.PATH:	${DIR_SA} ${DIR_KERN} \
	${DIR_LIBC}/inet ${DIR_LIBC}/quad ${DIR_LIBC}/string

# only needed during build
libinstall::

all realall: lib${LIB}.a SRT0.o

.include <bsd.own.mk>
.undef DESTDIR
.include <bsd.lib.mk>

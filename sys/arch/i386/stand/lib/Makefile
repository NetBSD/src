#	$NetBSD: Makefile,v 1.16 2000/05/13 19:58:41 jdolecek Exp $

S?=	${.CURDIR}/../../../../

LIB= i386
MKPIC=no
MKPROFILE=no

I386_INCLUDE_DISK?= yes
I386_INCLUDE_DOS?= no
I386_INCLUDE_BUS?= no

CPPFLAGS= -I$S/lib/libsa ${I386CPPFLAGS} ${I386MISCCPPFLAGS}
#CPPFLAGS+= -DDISK_DEBUG
#CPPFLAGS+= -DNO_DISKLABEL
#CPPFLAGS+= -DSAVE_MEMORY

SRCS= pcio.c conio.S comio.S comio_direct.c biosvideomode.S
SRCS+= getsecs.c biosgetrtc.S biosdelay.S biosreboot.S gatea20.c
SRCS+= biosmem.S getextmemx.c biosmemx.S printmemlist.c
SRCS+= pread.c menuutils.c parseutils.c
SRCS+= bootinfo.c bootinfo_biosgeom.c bootinfo_memmap.c
SRCS+= startprog.S netbsd_opts.c panic.c
.if (${I386_INCLUDE_DISK} == "yes")
SRCS+= biosdisk.c biosdisk_ll.c bios_disk.S
.endif
.if (${I386_INCLUDE_DOS} == "yes")
SRCS+= dosfile.c dos_file.S
.endif
.if (${I386_INCLUDE_DISK} == "yes") || (${I386_INCLUDE_DOS} == "yes")
SRCS+= diskbuf.c
.endif
.if (${I386_INCLUDE_BUS} == "yes")
SRCS+= biospci.c bios_pci.S isapnp.c isadma.c
.endif

.include <bsd.lib.mk>

lib${LIB}.o:: ${OBJS}
	@echo building standard ${LIB} library
	@rm -f lib${LIB}.o
	@${LD} -r -o lib${LIB}.o `lorder ${OBJS} | tsort`

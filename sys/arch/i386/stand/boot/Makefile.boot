# $NetBSD: Makefile.boot,v 1.23 2005/06/22 06:06:34 junyoung Exp $

S=	${.CURDIR}/../../../../../

NOMAN=
BINDIR= /usr/mdec
BINMODE= 0444
PROG?= boot
NEWVERSWHAT?= "BIOS Boot"
VERSIONFILE?= ${.CURDIR}/../version

SOURCES?= biosboot.S boot2.c conf.c devopen.c exec.c
SRCS= ${SOURCES}
.if !make(depend)
SRCS+= vers.c
.endif

.include <bsd.own.mk>

STRIPFLAG=	# nothing

LIBCRT0=	# nothing
LIBCRTBEGIN=	# nothing
LIBCRTEND=	# nothing
LIBC=		# nothing

BINDIR=/usr/mdec
BINMODE=444

.PATH:	${.CURDIR}/.. ${.CURDIR}/../../lib

LDFLAGS+= -N -e boot_start
# CPPFLAGS+= -D__daddr_t=int32_t
CPPFLAGS+= -I ${.CURDIR}/..  -I ${.CURDIR}/../../lib -I ${S}/lib/libsa
CPPFLAGS+= -I ${.OBJDIR}
#CPPFLAGS+= -DDEBUG_MEMSIZE

# Make sure we override any optimization options specified by the user
COPTS=  -Os

.if ${MACHINE} == "amd64"
LD+=  -m elf_i386
AFLAGS+=   -m32
CPUFLAGS=  -m32
LIBKERN_ARCH=i386
KERNMISCMAKEFLAGS="LIBKERN_ARCH=i386"
CPPFLAGS+= -DBOOT_ELF64
.else
CPUFLAGS=  -mcpu=i386
.endif

COPTS+=    -ffreestanding
CFLAGS+= -Wall -Wmissing-prototypes -Wstrict-prototypes
CPPFLAGS+= -nostdinc -D_STANDALONE
CPPFLAGS+= -I$S

CPPFLAGS+= -DSUPPORT_PS2
CPPFLAGS+= -DDIRECT_SERIAL
CPPFLAGS+= -DSUPPORT_SERIAL=boot_params.bp_consdev

CPPFLAGS+= -DCONSPEED=boot_params.bp_conspeed
CPPFLAGS+= -DCONSADDR=boot_params.bp_consaddr
CPPFLAGS+= -DCONSOLE_KEYMAP=boot_params.bp_keymap

CPPFLAGS+= -DSUPPORT_CD9660
CPPFLAGS+= -DSUPPORT_USTARFS
CPPFLAGS+= -DSUPPORT_DOSFS
CPPFLAGS+= -DPASS_BIOSGEOM
CPPFLAGS+= -DPASS_MEMMAP
#CPPFLAGS+= -DBOOTPASSWD
CPPFLAGS+= -DEPIA_HACK

# The biosboot code is linked to 'virtual' address of zero and is
# loaded at physical address 0x10000.
# XXX The heap values should be determined from _end.
SAMISCCPPFLAGS+= -DHEAP_START=0x20000 -DHEAP_LIMIT=0x50000
SAMISCMAKEFLAGS+= SA_USE_CREAD=yes	# Read compressed kernels
SAMISCMAKEFLAGS+= SA_INCLUDE_NET=no	# Netboot via TFTP, NFS


# CPPFLAGS+= -DBOOTXX_RAID1_SUPPORT

I386_STAND_DIR?= $S/arch/i386/stand

.if !make(obj) && !make(clean) && !make(cleandir)
.NOPATH: machine x86
.endif

depend dependall realdepend realall: machine x86 lib
CLEANFILES+= machine x86

machine::
	-rm -f $@
	ln -s $S/arch/i386/include $@

x86::
	-rm -f $@
	ln -s $S/arch/x86/include $@

${OBJS}: machine x86 lib

lib:
.ifdef LIBOBJ
	-rm -f $@
	ln -s ${LIBOBJ}/lib .
	[ -d ${LIBOBJ}/lib ] || mkdir ${LIBOBJ}/lib
.else
	mkdir lib
.endif

### find out what to use for libi386
I386DIR= ${I386_STAND_DIR}/lib
.include "${I386DIR}/Makefile.inc"
LIBI386= ${I386LIB}

### find out what to use for libsa
SA_AS= library
SAMISCMAKEFLAGS+="SA_USE_LOADFILE=yes"
.include "${S}/lib/libsa/Makefile.inc"
LIBSA= ${SALIB}

### find out what to use for libkern
KERN_AS= library
.include "${S}/lib/libkern/Makefile.inc"
LIBKERN= ${KERNLIB}

### find out what to use for libz
Z_AS= library
.include "${S}/lib/libz/Makefile.inc"
LIBZ= ${ZLIB}


cleandir distclean: cleanlibdir

cleanlibdir:
	-rm -rf lib

LIBLIST= ${LIBI386} ${LIBSA} ${LIBZ} ${LIBKERN} ${LIBI386} ${LIBSA}
# LIBLIST= ${LIBSA} ${LIBKERN} ${LIBI386} ${LIBSA} ${LIBZ} ${LIBKERN}

CLEANFILES+= ${PROG}.tmp ${PROG}.map vers.c

vers.c: ${VERSIONFILE} ${SOURCES} ${LIBLIST} ${.CURDIR}/../Makefile.boot
	${HOST_SH} ${S}conf/newvers_stand.sh ${VERSIONFILE} ${MACHINE} ${NEWVERSWHAT}

# Anything that calls 'real_to_prot' must have a %pc < 0x10000.
# We link the program, find the callers (all in libi386), then
# explicitely pull in the required objects before any other library code.
${PROG}: ${OBJS} ${LIBLIST} ${.CURDIR}/../Makefile.boot
	bb="$$( ${LD} -o ${PROG}.tmp ${LDFLAGS} -Ttext 0 -cref \
	    ${OBJS} ${LIBLIST} | ( \
		while read symbol file; do \
			[ -z "$$file" ] && continue; \
			[ "$$symbol" = real_to_prot ] && break; \
		done; \
		while \
			oifs="$$IFS"; \
			IFS='()'; \
			set -- $$file; \
			IFS="$$oifs"; \
			[ -n "$$2" ] && echo "${I386DST}/$$2"; \
			read file rest && [ -z "$$rest" ]; \
		do :; \
		done; \
	) )"; \
	${LD} -o ${PROG}.tmp ${LDFLAGS} -Ttext 0 \
		-Map ${PROG}.map -cref ${OBJS} $$bb ${LIBLIST}
	${OBJCOPY} -O binary ${PROG}.tmp ${PROG}
	rm -f ${PROG}.tmp

.include <bsd.prog.mk>

#	$NetBSD: Makefile,v 1.17 2001/01/04 09:10:25 itojun Exp $

LIB=		bfd

MKLINT=		no
MKMAN=		no
MKPROFILE=	no

# for MKPIC, BFD_SLIM, BFD_UNIFIED, OBJECT_FMT
.include <bsd.own.mk>

.if ${MKPIC} != "no"
MKLINKLIB=	no
.endif

.if (${OBJECT_FMT} == "ELF") || defined(BOOTSTRAP_ELF)
ELF_PLEASE=
.endif

# XXX BFD_UNIFIED should go away if benchmarking shows minor to no difference
.if ${MKPIC} == "no" || !defined(BFD_UNIFIED)
BFD_SLIM=
.endif

.if (${MACHINE_ARCH} == "alpha") || \
    (${MACHINE_ARCH} == "sparc64")
BFD64=
CPPFLAGS += -DBFD_TARG_64
.endif

DIST=		${.CURDIR}/../../dist

# generic bfd
SRCS=		archive.c archures.c bfd.c cache.c coffgen.c corefile.c \
		format.c init.c lib_bfd.c opncls.c reloc.c section.c syms.c \
		targets.c hash.c linker.c srec.c binary.c tekhex.c \
		ihex.c stabs.c stab-syms.c netbsd-core.c \
		dis-buf.c disassemble.c

# libiberty (yuck.)
SRCS+=		argv.c basename.c choose-temp.c concat.c cplus-dem.c \
		fdmatch.c fnmatch.c getopt.c getopt1.c getruntime.c hex.c \
		floatformat.c objalloc.c obstack.c pexecute.c spaces.c \
		strerror.c strsignal.c xatexit.c xexit.c xmalloc.c xstrdup.c \
		xstrerror.c insque.c

CPPFLAGS+= 	-DHAVE_CONFIG_H -I. -I${.CURDIR} \
		-I${DIST}/bfd -I${DIST}/opcodes -I${DIST}/include \
		${ARCHES:S/^/-DARCH_/} ${SELECT_VECS:S/^/-DHAVE_/} \
		-DSELECT_ARCHITECTURES="${ARCHES:S/^/,\&bfd_/:S/$/_arch/:S/^,//1}" \
		-DDEFAULT_VECTOR=${DEFAULT_VEC.${MACHINE_ARCH}} \
		-DSELECT_VECS="${SELECT_VECS:S/^/,\&/:S/^,//1}"

DPSRCS+=	bfd.h elf32-target.h elf64-target.h targmatch.h
CLEANFILES+=	bfd.h elf32-target.h elf64-target.h targmatch.h

TEXINFO=	bfd.texinfo
INFOFLAGS=	-I${DIST}/bfd/doc
SELECT_VECS=	netbsd_core_vec

.PATH:		${DIST}/bfd ${DIST}/opcodes ${DIST}/libiberty ${DIST}/include \
		${DIST}/bfd/doc

bfd.h: bfd-in2.h bfd-h.sed
	sed -f ${.ALLSRC:M*.sed} ${.ALLSRC:M*.h} >$@

targmatch.h: config.bfd targmatch.sed
	sed -f ${.ALLSRC:M*.sed} ${.ALLSRC:M*.bfd} >$@

elf32-target.h: elfxx-target.h
	sed -e s/NN/32/g < $> > $@

elf64-target.h: elfxx-target.h
	sed -e s/NN/64/g < $> > $@

##### alpha #####

.if defined(BFD64) && (${MACHINE_ARCH} == "alpha" || !defined(BFD_SLIM))
ARCHES+=		alpha
DEFAULT_VEC.alpha=	bfd_elf64_alpha_vec
SELECT_VECS+=		${DEFAULT_VEC.alpha} ecoffalpha_little_vec
SRCS+=			cpu-alpha.c elf64-alpha.c coff-alpha.c \
			alpha-dis.c alpha-opc.c ecoff.c ecofflink.c \
			elf64.c elf.c elflink.c dwarf2.c
.endif

##### arm32 #####

.if (${MACHINE_ARCH} == "arm32" || !defined(BFD_SLIM))
ARCHES+=		arm
DEFAULT_VEC.arm32=	armnetbsd_vec
SELECT_VECS+=		${DEFAULT_VEC.arm32}
SRCS+=			cpu-arm.c aout-arm32.c armnetbsd.c arm-dis.c
.endif

##### i386 #####

.if (${MACHINE_ARCH} == "i386" || !defined(BFD_SLIM))
ARCHES+=		i386
.if defined(ELF_PLEASE)
DEFAULT_VEC.i386=	bfd_elf32_i386_vec
SELECT_VECS+=		${DEFAULT_VEC.i386} i386netbsd_vec i386bsd_vec \
			i386freebsd_vec i386coff_vec
.else
DEFAULT_VEC.i386=	i386netbsd_vec
SELECT_VECS+=		${DEFAULT_VEC.i386} bfd_elf32_i386_vec i386bsd_vec \
			i386freebsd_vec i386coff_vec
.endif
SRCS+=			cpu-i386.c i386bsd.c i386freebsd.c i386netbsd.c \
			coff-i386.c elf32-i386.c i386-dis.c \
			aout32.c cofflink.c elf32.c elf.c elflink.c dwarf2.c \
			netbsdaout.c
.endif

##### m68k #####

.if (${MACHINE_ARCH} == "m68k" || !defined(BFD_SLIM))
ARCHES+=		m68k
.if defined(ELF_PLEASE)
DEFAULT_VEC.m68k=	bfd_elf32_m68k_vec
SELECT_VECS+=		${DEFAULT_VEC.m68k} m68knetbsd_vec \
			m68k4knetbsd_vec sunos_big_vec
.else
DEFAULT_VEC.m68k=	m68knetbsd_vec
SELECT_VECS+=		${DEFAULT_VEC.m68k} m68k4knetbsd_vec \
			bfd_elf32_m68k_vec sunos_big_vec
.endif
SRCS+=			cpu-m68k.c m68knetbsd.c m68k4knetbsd.c sunos.c \
			elf32-m68k.c m68k-dis.c m68k-opc.c \
			aout32.c elf32.c elf.c elflink.c dwarf2.c
.endif

##### mipse[bl] #####

.if (${MACHINE_ARCH} == "mipseb" || ${MACHINE_ARCH} == "mipsel" || \
     !defined(BFD_SLIM))
ARCHES+=		mips
DEFAULT_VEC.mipseb=	bfd_elf32_bigmips_vec
DEFAULT_VEC.mipsel=	bfd_elf32_littlemips_vec
SELECT_VECS+=		${DEFAULT_VEC.mipseb} ${DEFAULT_VEC.mipsel} \
			ecoff_big_vec ecoff_little_vec # mipselnetbsd_vec
SRCS+=			cpu-mips.c elf32-mips.c coff-mips.c \
			mips-dis.c mips-opc.c mips16-opc.c \
			ecoff.c ecofflink.c elf32.c elf.c elflink.c dwarf2.c
#			mipselnetbsd.c aout32.c
.endif

##### ns32k #####

.if (${MACHINE_ARCH} == "ns32k" || !defined(BFD_SLIM))
ARCHES+=		ns32k
DEFAULT_VEC.ns32k=	pc532netbsd_vec
SELECT_VECS+=		${DEFAULT_VEC.ns32k}
SRCS+=			ns32knetbsd.c aout-ns32k.c cpu-ns32k.c ns32k-dis.c
.endif

##### powerpc #####

.if (${MACHINE_ARCH} == "powerpc" || !defined(BFD_SLIM))
ARCHES+=		powerpc
DEFAULT_VEC.powerpc=	bfd_elf32_powerpc_vec
SELECT_VECS+=		${DEFAULT_VEC.powerpc} bfd_elf32_powerpcle_vec \
				rs6000coff_vec
SRCS+=			cpu-powerpc.c elf32-ppc.c ppc-dis.c ppc-opc.c \
			elf32.c elf.c elflink.c dwarf2.c coff-rs6000.c \
			xcofflink.c
.endif

##### sh3 #####

.if (${MACHINE_ARCH} == "sh3eb" || ${MACHINE_ARCH} == "sh3el" || !defined(BFD_SLIM))
ARCHES+=		sh
.if defined(ELF_PLEASE)
DEFAULT_VEC.sh3=	bfd_elf32_shunx_vec
SELECT_VECS+=		${DEFAULT_VEC.sh3} bfd_elf32_shlunx_vec \
			shcoff_vec shlcoff_vec
.else
DEFAULT_VEC.sh3=	shcoff_vec
SELECT_VECS+=		${DEFAULT_VEC.sh3} shlcoff_vec \
			bfd_elf32_shunx_vec bfd_elf32_shlunx_vec
.endif
SRCS+=			cpu-sh.c sh-dis.c \
			elf32-shunx.c elf32-sh.c elf32.c elf.c elflink.c \
			coff-sh.c cofflink.c coffgen.c dwarf2.c
.endif

##### sparc #####

.if (${MACHINE_ARCH} == "sparc" || ${MACHINE_ARCH} == "sparc64" || \
     !defined(BFD_SLIM))
ARCHES+=		sparc
.if (${MACHINE_ARCH} == "sparc") && defined(ELF_PLEASE)
DEFAULT_VEC.sparc=	bfd_elf32_sparc_vec
SELECT_VECS+=		${DEFAULT_VEC.sparc} ${DEFAULT_VEC.sparc64} \
			sunos_big_vec sparcnetbsd_vec
.else
DEFAULT_VEC.sparc=	sparcnetbsd_vec
SELECT_VECS+=		${DEFAULT_VEC.sparc} ${DEFAULT_VEC.sparc64} \
			sunos_big_vec bfd_elf32_sparc_vec
.endif
SRCS+=			cpu-sparc.c sparcnetbsd.c sunos.c elf32-sparc.c \
			sparc-dis.c sparc-opc.c \
			aout32.c elf32.c elf.c elflink.c dwarf2.c
.if defined(BFD64)
DEFAULT_VEC.sparc64=	bfd_elf64_sparc_vec
SRCS+=			elf64-sparc.c elf64.c
.endif
.endif

##### vax #####

.if (${MACHINE_ARCH} == "vax" || !defined(BFD_SLIM))
ARCHES+=		vax
DEFAULT_VEC.vax=	vaxnetbsd_vec
SELECT_VECS+=		${DEFAULT_VEC.vax} vaxcoff_vec
SRCS+=			cpu-vax.c vaxnetbsd.c vax-dis.c aout32.c coff-vax.c
.endif

# uniq-ify SRCS and SELECT_VECS

SRCS != (for i in ${SRCS}; do echo $$i; done) | sort | uniq
SELECT_VECS != (for i in ${SELECT_VECS}; do echo $$i; done) | sort | uniq

.include <bsd.info.mk>
.include <bsd.lib.mk>

${OBJS} ${SOBJS}: bfd.h elf32-target.h elf64-target.h
targets.o targets.so: targmatch.h Makefile

#	$NetBSD: Makefile,v 1.2 1999/01/15 10:57:37 castor Exp $

KDIR=	/sys/arch/pmax/include
INCSDIR= /usr/include/pmax

INCS=	ansi.h aout_machdep.h autoconf.h bsd-aout.h bus.h cdefs.h \
	clock_machdep.h conf.h cpu.h db_machdep.h dc7085cons.h disklabel.h \
	ecoff_machdep.h elf.h elf_machdep.h endian.h fbio.h fbvar.h float.h \
	ieeefp.h intr.h kcore.h kdbparam.h limits.h locore.h mips_opcode.h \
	param.h pcb.h pmap.h pmioctl.h proc.h profile.h psl.h pte.h ptrace.h \
	reg.h regdef.h regnum.h reloc.h setjmp.h signal.h stdarg.h sysconf.h \
	tc_machdep.h trap.h types.h varargs.h vmparam.h

DEPINCS= pubassym.h

.ifndef	S
S=	${.CURDIR}/../../..
.endif
MIPS=$S/arch/mips

CFLAGS+= -I. -D_LKM

pubassym.h:	${MIPS}/mips/genpubassym.cf
	ln -sf ${MIPS}/include mips && \
	ln -sf ${S}/sys sys  && \
	ln -sf ${S}/uvm uvm  && \
	ln -sf ${S}/arch/${MACHINE}/include machine && \
	sh ${S}/kern/genassym.sh ${CC} ${CFLAGS} ${CPPFLAGS} ${PROF} \
		< ${.ALLSRC} > pubassym.h.tmp && \
	mv -f pubassym.h.tmp ${.TARGET}

.include <bsd.kinc.mk>
.include <bsd.obj.mk>

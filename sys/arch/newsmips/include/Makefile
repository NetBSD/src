#	$NetBSD: Makefile,v 1.2 1999/01/15 10:57:36 castor Exp $

KDIR=	/sys/arch/newsmips/include
INCSDIR= /usr/include/newsmips

INCS=	adrsmap.h ansi.h aout_machdep.h autoconf.h bsd-aout.h cdefs.h conf.h \
	cpu.h db_machdep.h disklabel.h ecoff_machdep.h elf.h elf_machdep.h \
	endian.h float.h framebuf.h ieeefp.h intr.h kcore.h kdbparam.h \
	keyboard.h limits.h locore.h machConst.h mips_opcode.h mouse.h \
	param.h pcb.h pmap.h proc.h profile.h psl.h pte.h ptrace.h reg.h \
	regdef.h regnum.h reloc.h setjmp.h signal.h stdarg.h trap.h types.h \
	va-mips.h varargs.h vmparam.h z8530var.h

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
	mv -f pubassym.h.tmp pubassym.h

.include <bsd.kinc.mk>
.include <bsd.obj.mk>

# $NetBSD: Makefile,v 1.22 1999/02/10 16:15:21 tron Exp $

TOP=	$(.CURDIR)/../..
dist=	$(TOP)/dist
srcdir= $(dist)/gdb

.PATH:	$(srcdir)

.if	(${MACHINE_ARCH} != "powerpc") && (${MACHINE_ARCH} != "mipseb")

PROG=	gdb

BFDOBJ!=cd ${TOP}/lib/libbfd; \
        printf "xxx: .MAKE\n\t@echo \$${.OBJDIR}\n" | ${MAKE} -s -f-

# Note: nm.h, tm.h are in $(.OBJDIR),
# and config.h, xm.h are in $(.CURDIR)
INCLS+= -I$(.OBJDIR) -I$(.CURDIR)

# GDB_CFLAGS
INCLS+= -I$(srcdir) -I$(srcdir)/config

# READLINE_CFLAGS
INCLS+= -I$(dist)/readline

# BFD_CFLAGS: Make sure we get the installed bfd.h
INCLS+= -I${BFDOBJ}
INCLS+= -I$(dist)/bfd
INCLS+= -I$(dist)/include

.include <bsd.own.mk>
# misc. -DHAVE_TERMIOS
# KERNEL_DEBUG no longer used.  See ADDITIONAL_OPTIONS*
DEFS= -DNO_MMALLOC

.if ${OBJECT_FMT} == "ELF"
DEFS+= -DSVR4_SHARED_LIBS
.endif

CPPFLAGS+= $(INCLS) $(DEFS)

.include "_${MACHINE_ARCH:S/^mips$/mipsel/}.mk"

# Any NetBSD: (i386 m68k ns32k sparc vax)
NDEP_CMN?= infptrace.c inftarg.c fork-child.c corelow.c kcore-nbsd.c
XDEP_FILES= ser-tcp.c ser-unix.c
REMOTE_SRCS = remote.c dcache.c remote-utils.c

DEPFILES = $(TDEP_FILES) $(XDEP_FILES) \
 $(NDEP_CMN) $(NDEP_FILES) $(REMOTE_SRCS)

YYSRCS= c-exp.y f-exp.y m2-exp.y

# This exists only for easy comparison with the GNU Makefile.
COMMON_SRCS = version.c blockframe.c breakpoint.c findvar.c stack.c thread.c \
	source.c values.c eval.c valops.c valarith.c valprint.c printcmd.c \
	symtab.c symfile.c symmisc.c infcmd.c infrun.c command.c \
	expprint.c environ.c gdbtypes.c copying.c $(DEPFILES) \
	mem-break.c target.c parse.c language.c $(YYSRCS) buildsym.c \
	exec.c bcache.c objfiles.c minsyms.c maint.c demangle.c \
	dbxread.c coffread.c elfread.c \
	dwarfread.c mipsread.c stabsread.c corefile.c \
	c-lang.c ch-exp.c ch-lang.c f-lang.c m2-lang.c \
	scm-exp.c scm-lang.c scm-valprint.c complaints.c typeprint.c \
	c-typeprint.c ch-typeprint.c f-typeprint.c m2-typeprint.c \
	c-valprint.c cp-valprint.c ch-valprint.c f-valprint.c m2-valprint.c \
	nlmread.c serial.c mdebugread.c os9kread.c top.c utils.c callback.c

SRCS=	main.c $(COMMON_SRCS) annotate.c inflow.c gnu-regex.c
OBJS=	init.o

LDADD+= -ledit
DPADD+= $(LIBEDIT)

LDADD+= -L${BFDOBJ} -lbfd
DPADD+= ${BFDOBJ}/libbfd.a

LDADD+= -lkvm
DPADD+= $(LIBKVM)

LDADD+= -ltermcap
DPADD+= $(LIBTERMCAP)

DPSRCS+= nm.h tm.h

.endif

MAN=	gdb.1

.include <bsd.prog.mk>

# Note: this rule does something similar to the gcc "collect" program,
# by scanning all the objects for initialization functions, collecting
# them all into the generated init.c file, which is called by main.
CLEANFILES+= init.c
init.o : $(OBJS:S/init.o//)
	-rm -f init.c
	sh $(.CURDIR)/mkinit.sh $(OBJS:S/init.o//) > init.c
	${COMPILE.c} init.c

# Need these headers before any objects.
$(OBJS) : nm.h tm.h xm.h

nm.h : $(srcdir)/$(NM_FILE)
	-rm -f $@
	cp -p $(srcdir)/$(NM_FILE) $@

tm.h : $(srcdir)/$(TM_FILE)
	-rm -f $@
	cp -p $(srcdir)/$(TM_FILE) $@

version.o : version.c
	$(CC) $(CFLAGS) $(CPPFLAGS) \
	  -DHOST_NAME=\"${MACHINE_GNU_ARCH}--netbsd\" \
	  -DTARGET_NAME=\"${MACHINE_GNU_ARCH}--netbsd\" \
	  -c $(.IMPSRC)

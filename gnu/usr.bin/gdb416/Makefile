# $NetBSD: Makefile,v 1.3 1997/09/26 18:09:58 gwr Exp $

# PROG=	gdb # No, this needs custom rules.
# Use <bsd.prog.mk> somehow?
PROGNAME=gdb

TOP=	$(.CURDIR)/../..
dist=	$(TOP)/dist
srcdir= $(dist)/gdb

.PATH:	$(srcdir)

# Override this to build a cross-debugger.
TARGET_ARCH?=$(MACHINE_ARCH)

# Compiler options.
CFLAGS=-g

# Find out where libreadline.a lives.
RL_CURDIR=$(TOP)/lib/readline
RL_OBJDIR!=cd $(RL_CURDIR); $(MAKE) echo_objdir
LIBREADLINE=$(RL_OBJDIR)/libreadline.a

# Note: nm.h, tm.h are in $(.OBJDIR),
# and config.h, xm.h are in $(.CURDIR)
INCLS+= -I$(.OBJDIR) -I$(.CURDIR)

# GDB_CFLAGS
INCLS+= -I$(srcdir) -I$(srcdir)/config

# READLINE_CFLAGS
INCLS+= -I$(dist)/readline

# BFD_CFLAGS: Make sure we get the installed bfd.h
INCLS+= -I$(TOP)/lib/bfd
INCLS+= -I$(dist)/bfd
INCLS+= -I$(dist)/include

# misc. -DHAVE_TERMIOS
# KERNEL_DEBUG no longer used.  See ADDITIONAL_OPTIONS*
DEFS= -DNO_MMALLOC

CPPFLAGS= $(INCLS) $(DEFS)

.include "_$(TARGET_ARCH).mk"

# Any NetBSD: (i386 m68k ns32k sparc vax)
NDEP_CMN?= infptrace.o inftarg.o fork-child.o corelow.o kcore-nbsd.o
XDEP_FILES= ser-tcp.o ser-unix.o
REMOTE_OBS = remote.o dcache.o remote-utils.o

DEPFILES = $(TDEP_FILES) $(XDEP_FILES) \
 $(NDEP_CMN) $(NDEP_FILES) $(REMOTE_OBS)

YYOBJ= c-exp.tab.o f-exp.tab.o m2-exp.tab.o

# This exists only for easy comparison with the GNU Makefile.
COMMON_OBS = version.o blockframe.o breakpoint.o findvar.o stack.o thread.o \
	source.o values.o eval.o valops.o valarith.o valprint.o printcmd.o \
	symtab.o symfile.o symmisc.o infcmd.o infrun.o command.o \
	expprint.o environ.o gdbtypes.o copying.o $(DEPFILES) \
	mem-break.o target.o parse.o language.o $(YYOBJ) buildsym.o \
	exec.o bcache.o objfiles.o minsyms.o maint.o demangle.o \
	dbxread.o coffread.o elfread.o \
	dwarfread.o mipsread.o stabsread.o corefile.o \
	c-lang.o ch-exp.o ch-lang.o f-lang.o m2-lang.o \
	scm-exp.o scm-lang.o scm-valprint.o complaints.o typeprint.o \
	c-typeprint.o ch-typeprint.o f-typeprint.o m2-typeprint.o \
	c-valprint.o cp-valprint.o ch-valprint.o f-valprint.o m2-valprint.o \
	nlmread.o serial.o mdebugread.o os9kread.o top.o utils.o callback.o

OBJS =	main.o $(COMMON_OBS) annotate.o inflow.o gnu-regex.o

LDADD+= $(LIBREADLINE)
DPADD+= $(LIBREADLINE)

LDADD+= -lbfd
DPADD+= ${DESTDIR}/usr/lib/libbfd.a

LDADD+= -lkvm
DPADD+= $(LIBKVM)

LDADD+= -ltermcap
DPADD+= $(LIBTERMCAP)

# Note custom rules - did NOT set PROG
CLEANFILES+= gdb
all: gdb

.include <bsd.prog.mk>

CLEANFILES+= init.c init.o
gdb : $(OBJS) $(DPADD)
	-rm -f init.c
	sh $(.CURDIR)/mkinit.sh $(OBJS) > init.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c init.c
	$(CC) -o $@ $(OBJS) init.o $(LDADD)

# Need these headers before any objects.
$(OBJS) : nm.h tm.h xm.h

CLEANFILES+= nm.h
nm.h : $(srcdir)/$(NM_FILE)
	-rm -f $@
	cp -p $(srcdir)/$(NM_FILE) $@

CLEANFILES+= tm.h
tm.h : $(srcdir)/$(TM_FILE)
	-rm -f $@
	cp -p $(srcdir)/$(TM_FILE) $@

version.o : version.c
	$(CC) $(CFLAGS) $(CPPFLAGS) \
	  -DHOST_NAME=\"$(MACHINE_ARCH)-netbsd\" \
	  -DTARGET_NAME=\"$(TARGET_ARCH)-netbsd\" \
	  -c $(.IMPSRC)

CLEANFILES+= c-exp.tab.h c-exp.c
c-exp.c c-exp.tab.h : c-exp.y
	${YACC} -d -p c_ -b c-exp $(.IMPSRC)
c-exp.o : c-exp.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $(.IMPSRC)

CLEANFILES+= f-exp.tab.h f-exp.c
f-exp.c f-exp.tab.h : f-exp.y
	${YACC} -d -p f_ -b f-exp $(.IMPSRC)
f-exp.o : f-exp.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $(.IMPSRC)

CLEANFILES+= m2-exp.tab.h m2-exp.c
m2-exp.c m2-exp.tab.h : m2-exp.y
	${YACC} -d -p m2_ -b m2-exp $(.IMPSRC)
m2-exp.o : m2-exp.c
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $(.IMPSRC)

cleandepend:
	-rm -f .depend

DPSRCS = ${OBJS:.o=.c} nm.h tm.h
depend: .depend
.depend: ${DPSRCS}
	${CC} -MM $(CPPFLAGS) ${.ALLSRC:M*.c} > .depend.tmp
	-mv -f .depend.tmp $@

proginstall:: ${DESTDIR}${BINDIR}/${PROGNAME}
.if !defined(UPDATE)
.PHONY: ${DESTDIR}${BINDIR}/${PROGNAME}
.endif
.if !defined(BUILD)
${DESTDIR}${BINDIR}/${PROGNAME}: .MADE
.endif

.PRECIOUS: ${DESTDIR}${BINDIR}/${PROGNAME}
${DESTDIR}${BINDIR}/${PROGNAME}: ${PROGNAME}
	${INSTALL} ${COPY} ${STRIPFLAG} -o ${BINOWN} -g ${BINGRP} \
	    -m ${BINMODE} ${.ALLSRC} ${.TARGET}


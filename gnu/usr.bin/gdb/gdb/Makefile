# $Id: Makefile,v 1.2 1994/01/28 21:37:23 cgd Exp $

# Paths to libraries
#
LIBBFD!=cd $(.CURDIR)/../bfd; \
	printf "xxx:\n\techo \$${.OBJDIR}/libbfd.a\n" | make -r -s -f - xxx

LIBREADLINE!=cd $(.CURDIR)/../readline; \
	printf "xxx:\n\techo \$${.OBJDIR}/libreadline.a\n" | make -r -s -f - xxx

LIBIBERTY!=cd $(.CURDIR)/../libiberty; \
	printf "xxx:\n\techo \$${.OBJDIR}/libiberty.a\n" | make -r -s -f - xxx


PROG=		gdb

CFLAGS+=	-I$(.CURDIR)/arch/$(MACHINE_ARCH) -I$(.CURDIR)/../bfd \
		-I$(.CURDIR)/../readline -I$(.CURDIR)/../include \
		-I$(.CURDIR) \
		-DHOST_CANONICAL=\"$(MACHINE_ARCH)-netbsd\" \
		-DTARGET_CANONICAL=\"$(MACHINE_ARCH)-netbsd\"

CFLAGS+=	-DNO_MMALLOC
YFLAGS+=	-p $(.PREFIX:S/-exp/_/:S/ch_/chill_/)

.PATH:		$(.CURDIR)/arch/$(MACHINE_ARCH)

SRCS=		blockframe.c core.c gdbtypes.c main.c source.c		\
		breakpoint.c maint.c  stabsread.c			\
		buildsym.c corelow.c mem-break.c stack.c		\
		c-exp.c cp-valprint.c minsyms.c symfile.c		\
		c-lang.c dbxread.c elfread.c infcmd.c symmisc.c		\
		c-typeprint.c dcache.c inflow.c symtab.c		\
		c-valprint.c demangle.c infptrace.c objfiles.c target.c	\
		ch-exp.c infrun.c parse.c dwarfread.c thread.c		\
		ch-lang.c inftarg.c printcmd.c typeprint.c		\
		ch-typeprint.c environ.c init.c utils.c			\
		ch-valprint.c eval.c  language.c regex.c  valarith.c	\
		exec.c  m2-exp.c remote-utils.c valops.c		\
		command.c expprint.c m2-lang.c remote.c valprint.c	\
		complaints.c findvar.c m2-typeprint.c ser-unix.c values.c\
		copying.c fork-child.c m2-valprint.c serial.c version.c \
		dis-buf.c solib.c

LDADD=		$(LIBBFD) $(LIBREADLINE) $(LIBIBERTY)
DPADD=		$(LIBBFD) $(LIBREADLINE) $(LIBIBERTY)

LDADD+=		-ltermcap
DPADD+=		$(LIBTERMCAP)


.include "arch/$(MACHINE_ARCH)/Makefile.inc"
.include "../../Makefile.inc"
.include <bsd.prog.mk>


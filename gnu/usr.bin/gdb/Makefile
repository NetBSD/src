#	from: @(#)Makefile	6.4 (Berkley) 5/6/91
#	$Id: Makefile,v 1.5 1993/12/05 00:58:46 cgd Exp $

PROG=		gdb
GDBSRCS=	blockframe.c breakpoint.c command.c copying.c core.c \
		cplus-dem.c dbxread.c environ.c eval.c expprint.c \
		expread.y findvar.c infcmd.c inflow.c infrun.c \
		main.c obstack.c printcmd.c regex.c remote.c \
		remote-sl.c source.c stack.c symmisc.c symtab.c \
		utils.c valarith.c valops.c valprint.c values.c \
		version.c
READLINESRCS=	funmap.c history.c keymaps.c readline.c
SRCS=		param.h $(CONFIGSRCS) $(GDBSRCS) $(READLINESRCS) init.c
CFLAGS+=	-I. -I$(.CURDIR) -I$(.CURDIR)/config -I$(.CURDIR)/readline \
		-DHAVE_VPRINTF -DVI_MODE -DKERNELDEBUG -DNEWVM
LDADD+=		-ltermcap -gnumalloc
DPADD+=		${LIBTERMCAP} /usr/lib/libgnumalloc.a
YFLAGS=
.PATH: $(.CURDIR)/config $(.CURDIR)/readline

.include "config/Makefile.$(MACHINE_ARCH)"
.include <bsd.prog.mk>

$(OBJS):	param.h

#
# Generate the constructor
#
init.c:		$(CONFIGSRCS) $(GDBSRCS) $(READLINESRCS)
	-((cd $(.CURDIR)/config; \
		egrep -h '^_initialize_[^ ]* *\(\)' $(CONFIGSRCS)); \
	  (cd $(.CURDIR); egrep -h '^_initialize_[^ ]* *\(\)' $(GDBSRCS)); \
	  (cd $(.CURDIR)/readline; \
		egrep -h '^_initialize_[^ ]* *\(\)' $(READLINESRCS))) | \
	(echo 'void initialize_all_files () {'; sed -e 's/$$/;/'; echo '}') \
		> init.c

CLEANFILES+=	init.c param.h

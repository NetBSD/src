#	$Id: Makefile.inc,v 1.5 1993/12/28 11:59:20 davidb Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

NETBOOTDIR=	$S/lib/libnetboot

NETBOOTLIBDIR!=cd $(NETBOOTDIR); \
	printf "xxx:\n\techo \$${.OBJDIR}\n" | make -r -s -f - xxx

NETBOOTLIB=	$(NETBOOTLIBDIR)/libnetboot.a

$(NETBOOTLIB):	.NOTMAIN __always_make_netbootlib
	@echo making sure the netboot library is up to date...
	@(cd $(NETBOOTDIR) ; make)

__always_make_netbootlib: .NOTMAIN

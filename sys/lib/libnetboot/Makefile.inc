#	$NetBSD: Makefile.inc,v 1.6 1994/10/26 06:42:59 cgd Exp $
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

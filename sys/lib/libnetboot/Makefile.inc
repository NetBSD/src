#	$Id: Makefile.inc,v 1.3 1993/10/13 06:17:33 cgd Exp $
#
#	NOTE: $S must correspond to the top of the 'sys' tree

NETBOOTDIR=	$S/lib/libnetboot

.if exists($(NETBOOTDIR)/obj)
NETBOOTLIBDIR=	$(NETBOOTDIR)/obj
.else
NETBOOTLIBDIR=  $(NETBOOTDIR)
.endif

NETBOOTLIB=	$(NETBOOTLIBDIR)/libnetboot.a

$(NETBOOTLIB):	.NOTMAIN __always_make_netbootlib
	@echo making sure the netboot library is up to date...
	@(cd $(NETBOOTDIR) ; make)

__always_make_netbootlib: .NOTMAIN

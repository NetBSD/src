#	$Id: Makefile.inc,v 1.1 1993/10/13 05:41:19 cgd Exp $

NETBOOTDIR=	$S/lib/libnetboot

.if exists($(NETBOOTDIR)/obj)
NETBOOTLIBDIR=	$(NETBOOTDIR)/obj
.else
NETBOOTLIBDIR=  $(NETBOOTDIR)
.endif

NETBOOTLIB=	$(NETBOOTLIBDIR)/libnetboot.a

$(NETBOOTLIB):	__always_make_netbootlib
	@echo making sure the netboot library is up to date...
	@(cd $(NETBOOTDIR) ; make)

__always_make_netbootlib:

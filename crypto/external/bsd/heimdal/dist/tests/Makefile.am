# $Id: Makefile.am,v 1.1.1.1 2011/04/13 18:16:01 elric Exp $

include $(top_srcdir)/Makefile.am.common

SUBDIRS = bin db kdc gss ldap can java

if ENABLE_SHARED
if HAVE_DLOPEN
SUBDIRS += plugin
endif
endif

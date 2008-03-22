# $Heimdal: Makefile.am 21418 2007-07-05 13:55:37Z lha $
# $NetBSD: Makefile.am,v 1.1 2008/03/22 09:29:56 mlelstv Exp $

include $(top_srcdir)/Makefile.am.common

SUBDIRS = db kdc gss ldap can java

if ENABLE_SHARED
if HAVE_DLOPEN
SUBDIRS += plugin
endif
endif

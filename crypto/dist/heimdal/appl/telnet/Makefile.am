# $Heimdal: Makefile.am 5652 1999-03-20 13:58:20Z joda $
# $NetBSD: Makefile.am,v 1.2 2008/03/22 08:36:55 mlelstv Exp $

include $(top_srcdir)/Makefile.am.common

SUBDIRS = libtelnet telnet telnetd

dist-hook:
	$(mkinstalldirs) $(distdir)/arpa
	$(INSTALL_DATA) $(srcdir)/arpa/telnet.h $(distdir)/arpa

EXTRA_DIST = README.ORIG telnet.state

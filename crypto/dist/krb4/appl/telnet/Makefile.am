# $Id: Makefile.am,v 1.1.1.2 2000/12/29 01:42:32 assar Exp $

include $(top_srcdir)/Makefile.am.common

SUBDIRS = libtelnet telnet telnetd

dist-hook:
	$(mkinstalldirs) $(distdir)/arpa
	$(INSTALL_DATA) $(srcdir)/arpa/telnet.h $(distdir)/arpa

EXTRA_DIST = README.ORIG telnet.state

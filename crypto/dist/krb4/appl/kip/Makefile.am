# $KTH-KRB: Makefile.am,v 1.4 2002/02/06 02:58:45 assar Exp $
# $NetBSD: Makefile.am,v 1.1.1.2 2002/09/12 12:22:04 joda Exp $

include $(top_srcdir)/Makefile.am.common

INCLUDES += $(INCLUDE_des) -DLIBEXECDIR="\"$(libexecdir)\""

bin_PROGRAMS = kip
libexec_PROGRAMS = kipd
libexec_SCRIPTS = kip-join-network kipd-control
CLEANFILES = $(libexec_SCRIPTS)

kip_SOURCES = kip.c common.c
kipd_SOURCES = kipd.c common.c

LDADD = \
	$(top_builddir)/lib/krb/libkrb.la	\
	$(LIB_des)	\
	$(LIB_roken)

kip-join-network: kip-join-network.in
	sed -e "s!%bindir%!$(bindir)!" $(srcdir)/kip-join-network.in > $@
	chmod +x $@

kipd-control: kipd-control.in
	sed -e "s!%bindir%!$(bindir)!" $(srcdir)/kipd-control.in > $@
	chmod +x $@


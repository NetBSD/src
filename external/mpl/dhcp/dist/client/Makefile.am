# We want to build this directory first, before descending into tests subdir.
# The reason is that ideally the tests should link existing objects from this
# directory. That eliminates any discrepancies between tested code and
# production code. Sadly, we are not there yet.
SUBDIRS = . tests

AM_CPPFLAGS = -DCLIENT_PATH='"PATH=$(sbindir):/sbin:/bin:/usr/sbin:/usr/bin"'
AM_CPPFLAGS += -DLOCALSTATEDIR='"$(localstatedir)"'

dist_sysconf_DATA = dhclient.conf.example
sbin_PROGRAMS = dhclient
dhclient_SOURCES = client_tables.c clparse.c dhclient.c dhc6.c \
		   scripts/bsdos scripts/freebsd scripts/linux scripts/macos \
		   scripts/netbsd scripts/nextstep scripts/openbsd \
		   scripts/solaris scripts/openwrt
dhclient_LDADD = ../common/libdhcp.@A@ ../omapip/libomapi.@A@ \
		 @BINDLIBIRSDIR@/libirs.@A@ \
		 @BINDLIBDNSDIR@/libdns.@A@ \
		 @BINDLIBISCCFGDIR@/libisccfg.@A@ \
		 @BINDLIBISCDIR@/libisc.@A@
man_MANS = dhclient.8 dhclient-script.8 dhclient.conf.5 dhclient.leases.5
EXTRA_DIST = $(man_MANS)

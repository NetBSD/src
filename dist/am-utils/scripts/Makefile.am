## Process this file with automake to produce Makefile.in

# Package: am-utils
# Level: Makefile for scripts/ directory
# Author: Erez Zadok

sbin_SCRIPTS =		\
	am-eject	\
	amd2ldif	\
	amd2sun		\
	automount2amd	\
	ctl-amd		\
	ctl-hlfsd	\
	fixrmtab	\
	fix-amd-map	\
	lostaltmail	\
	wait4amd	\
	wait4amd2die

bin_SCRIPTS = expn

sysconf_DATA = amd.conf-sample lostaltmail.conf-sample

# man pages
man_MANS = amd.conf.5 automount2amd.8 expn.1

EXTRA_DIST = 			\
	am-eject.in		\
	amd2ldif.in		\
	amd2sun.in		\
	automount2amd.in	\
	ctl-amd.in		\
	ctl-hlfsd.in		\
	fixrmtab.in		\
	fix-amd-map.in		\
	lostaltmail.in		\
	wait4amd.in		\
	wait4amd2die.in		\
	\
	expn.in			\
	\
	amd.conf-sample		\
	lostaltmail.conf-sample \
	$(man_MANS)

CLEANFILES = $(sbin_SCRIPTS) $(bin_SCRIPTS)

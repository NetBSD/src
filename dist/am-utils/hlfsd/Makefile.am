## Process this file with automake to produce Makefile.in

# Package: am-utils
# Level: Makefile for hlfsd/ directory
# Author: Erez Zadok

sbin_PROGRAMS = hlfsd

# man pages
man_MANS = hlfsd.8

# headers this depends on, not to be installed
noinst_HEADERS = hlfsd.h

hlfsd_SOURCES = 	\
	hlfsd.c		\
	homedir.c	\
	nfs_prot_svc.c	\
	stubs.c

LDADD = ../libamu/libamu.la

INCLUDES = -I$(top_srcdir)/include

# allow users to add their own flags via "configure --enable-am-flags=ARG"
AMU_CFLAGS = @AMU_CFLAGS@
CFLAGS = @CFLAGS@ $(AMU_CFLAGS)

# additional files to distribute and clean
EXTRA_DIST = $(man_MANS)

# dependencies
$(PROGRAMS): $(LDADD)
$(OBJECTS):					\
	../config.h				\
	../aux_conf.h				\
	$(top_srcdir)/include/am_compat.h	\
	$(top_srcdir)/include/am_defs.h		\
	$(top_srcdir)/include/am_utils.h	\
	$(top_srcdir)/include/am_xdr_func.h	\
	$(top_srcdir)/include/amq_defs.h	\
	@AMU_NFS_PROT_HEADER@			\
	$(noinst_HEADERS)

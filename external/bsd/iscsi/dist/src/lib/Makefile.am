## $NetBSD: Makefile.am,v 1.2 2009/06/30 02:44:52 agc Exp $

AM_CFLAGS		= $(WARNCFLAGS)

lib_LTLIBRARIES		= libiscsi.la

libiscsi_la_CPPFLAGS	= -I$(top_srcdir)/include

libiscsi_la_SOURCES	= \
	conffile.c \
	disk.c \
	initiator.c \
	md5c.c \
	md5hl.c \
	netmask.c \
	parameters.c \
	protocol.c \
	storage.c \
	snprintf.c \
	strlcpy.c \
	strtoll.c \
	target.c \
	util.c \
	uuid.c

man3_MANS		= libiscsi.3

dist_man_MANS		= libiscsi.3

## $NetBSD: Makefile.am,v 1.1 2009/06/25 13:47:11 agc Exp $

AM_CFLAGS		= $(WARNCFLAGS)

lib_LTLIBRARIES		= libiscsi.la

libiscsi_la_CPPFLAGS	= -I$(top_srcdir)/include

libiscsi_la_SOURCES	= \
	conffile.c \
	disk.c \
	initiator.c \
	iscsi.c \
	md5c.c \
	md5hl.c \
	netmask.c \
	parameters.c \
	storage.c \
	snprintf.c \
	strlcpy.c \
	strtoll.c \
	target.c \
	util.c \
	uuid.c

man3_MANS		= libiscsi.3

dist_man_MANS		= libiscsi.3

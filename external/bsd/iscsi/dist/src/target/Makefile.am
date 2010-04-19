## $NetBSD: Makefile.am,v 1.3 2010/04/19 08:05:36 sborrill Exp $

AM_CFLAGS		= $(WARNCFLAGS) -pthread

bin_PROGRAMS		= iscsi-target

iscsi_target_SOURCES	= iscsi-target.c

iscsi_target_CPPFLAGS	= -I$(top_srcdir)/include

iscsi_target_LDADD	= ../lib/libiscsi.la -lpthread

man8_MANS		= iscsi-target.8
man5_MANS		= targets.5

dist_man_MANS		= iscsi-target.8 targets.5

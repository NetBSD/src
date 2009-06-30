## $NetBSD: Makefile.am,v 1.2 2009/06/30 02:44:52 agc Exp $

AM_CFLAGS		= $(WARNCFLAGS)

bin_PROGRAMS		= iscsi-initiator

iscsi_initiator_SOURCES	= iscsi-initiator.c initiator.c libkmod.c virtdir.c

iscsi_initiator_CPPFLAGS= -I$(top_srcdir)/include

iscsi_initiator_LDADD	= ../lib/libiscsi.la -lrefuse

man1_MANS		= iscsi-initiator.8

dist_man_MANS		= iscsi-initiator.8

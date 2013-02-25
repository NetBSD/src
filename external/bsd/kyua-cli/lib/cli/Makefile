# $NetBSD: Makefile,v 1.1.4.2 2013/02/25 00:26:20 tls Exp $

KYUA_LIBS=	engine store utils

.include <bsd.init.mk>

.PATH:		${SRCDIR}/cli

LIB=		cli

WARNS?=		3

SRCS=		cmd_about.cpp \
		cmd_config.cpp \
		cmd_db_exec.cpp \
		cmd_db_migrate.cpp \
		cmd_debug.cpp \
		cmd_help.cpp \
		cmd_list.cpp \
		cmd_report.cpp \
		cmd_report_html.cpp \
		cmd_test.cpp \
		common.cpp \
		config.cpp \
		main.cpp

.include <bsd.lib.mk>

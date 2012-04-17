#
# Makefile.am
#
#       The Initial Developer of the Original Code is International
#       Business Machines Corporation. Portions created by IBM
#       Corporation are Copyright (C) 2005, 2006 International Business
#       Machines Corporation. All Rights Reserved.
#
#       This program is free software; you can redistribute it and/or modify
#       it under the terms of the Common Public License as published by
#       IBM Corporation; either version 1 of the License, or (at your option)
#       any later version.
#
#       This program is distributed in the hope that it will be useful,
#       but WITHOUT ANY WARRANTY; without even the implied warranty of
#       MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#       Common Public License for more details.
#
#       You should have received a copy of the Common Public License
#       along with this program; if not, a copy can be viewed at
#       http://www.opensource.org/licenses/cpl1.0.php.
#

noinst_HEADERS	=	tpm_nvcommon.h

noinst_PROGRAMS =	tpm_startup	\
			tpm_reset

sbin_PROGRAMS	=	tpm_changeownerauth \
			tpm_clear	\
			tpm_createek	\
			tpm_getpubek    \
			tpm_restrictpubek \
			tpm_setactive	\
			tpm_setenable	\
			tpm_setclearable \
			tpm_setownable	\
			tpm_setpresence	\
			tpm_takeownership \
			tpm_version	\
			tpm_selftest

if TSS_LIB_IS_12
sbin_PROGRAMS  += 	tpm_nvdefine    \
			tpm_nvinfo      \
			tpm_nvread      \
			tpm_nvrelease   \
			tpm_nvwrite     \
			tpm_resetdalock \
			tpm_restrictsrk \
			tpm_revokeek \
			tpm_setoperatorauth
AM_CPPFLAGS	=	-I$(top_srcdir)/include -D_LINUX -DTSS_LIB_IS_12
else
AM_CPPFLAGS	=	-I$(top_srcdir)/include -D_LINUX
endif

LDADD		=	$(top_builddir)/lib/libtpm_tspi.la -ltspi

tpm_startup_SOURCES =		tpm_startup.c
tpm_reset_SOURCES =		tpm_reset.c

tpm_changeownerauth_SOURCES =	tpm_changeauth.c
tpm_clear_SOURCES	=	tpm_clear.c
tpm_createek_SOURCES	=	tpm_createek.c
tpm_getpubek_SOURCES	=	tpm_getpubek.c
tpm_nvdefine_SOURCES	=	tpm_nvdefine.c tpm_nvcommon.c
tpm_nvinfo_SOURCES	=	tpm_nvinfo.c tpm_nvcommon.c
tpm_nvrelease_SOURCES	=	tpm_nvrelease.c tpm_nvcommon.c
tpm_nvread_SOURCES	=	tpm_nvread.c tpm_nvcommon.c
tpm_nvwrite_SOURCES	=	tpm_nvwrite.c tpm_nvcommon.c
tpm_restrictpubek_SOURCES =	tpm_restrictpubek.c
tpm_setactive_SOURCES	=	tpm_activate.c
tpm_setclearable_SOURCES =	tpm_clearable.c
tpm_setenable_SOURCES	=	tpm_enable.c
tpm_setownable_SOURCES	=	tpm_ownable.c
tpm_setpresence_SOURCES =	tpm_present.c
tpm_takeownership_SOURCES =	tpm_takeownership.c
tpm_version_SOURCES	=	tpm_version.c
tpm_selftest_SOURCES	=	tpm_selftest.c
tpm_revokeek_SOURCES	=	tpm_revokeek.c
tpm_setoperatorauth_SOURCES =   tpm_setoperatorauth.c
tpm_resetdalock_SOURCES =       tpm_resetdalock.c
tpm_restrictsrk_SOURCES =       tpm_restrictsrk.c


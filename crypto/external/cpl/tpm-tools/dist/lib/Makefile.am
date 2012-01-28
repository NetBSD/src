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

# Gettext support
localedir		=	$(datadir)/locale

# Libraries to build
lib_LTLIBRARIES		= 	libtpm_unseal.la 
noinst_LTLIBRARIES	=	libtpm_tspi.la	\
				libtpm_utils.la

#
# Common build flags (with gettext support)
AM_CPPFLAGS		=	-I$(top_srcdir)/include -D_LINUX -DLOCALEDIR=\"$(localedir)\"

if TSS_LIB_IS_12
AM_CPPFLAGS		+=	-DTSS_LIB_IS_12
endif

#
# Utility library
libtpm_utils_la_SOURCES	=	tpm_utils.c \
				tpm_log.c

#
# TSPI interface library
libtpm_tspi_la_SOURCES	=	tpm_tspi.c
libtpm_tspi_la_LIBADD	=	libtpm_utils.la -ldl

#
# PKCS#11 interface library
if P11_SUPPORT
noinst_LTLIBRARIES	+=	libtpm_pkcs11.la

libtpm_pkcs11_la_SOURCES=	tpm_pkcs11.c
libtpm_pkcs11_la_LIBADD	=	libtpm_utils.la
endif

#
# Installed Libraries

#
# Libtool -version-info flag usage rules
# This flag accepts an argument of the form current[:revision[:age]]. 
# So, passing -version-info 3:12:1 sets current to 3, revision to 12, and age to 1.
# If either revision or age are omitted, they default to 0. Also note that age must 
# be less than or equal to the current interface number.
# Here are a set of rules to help you update your library version information:
#   1. Start with version information of 0:0:0 for each libtool library.
#   2. Update the version information only immediately before a public release of your 
#	software. More frequent updates are unnecessary, and only guarantee that the 
#	current interface number gets larger faster.
#   3. If the library source code has changed at all since the last update, then 
#	increment revision (c:r:a becomes c:r+1:a).
#   4. If any interfaces have been added, removed, or changed since the last update, 
#	increment current, and set revision to 0.
#   5. If any interfaces have been added since the last public release, then increment age.
#   6. If any interfaces have been removed since the last public release, then set age to 0.  

#
# Unseal library - for addition to existing apps, counter part of seal cmd
libtpm_unseal_la_SOURCES =	tpm_unseal.c
libtpm_unseal_la_LDFLAGS =	-shared -version-info 1:0:0
libtpm_unseal_la_LIBADD  =	-ltspi  libtpm_tspi.la

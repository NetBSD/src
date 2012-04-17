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

bin_PROGRAMS 	=	tpm_sealdata \
			tpm_unsealdata

if TSS_LIB_IS_12
AM_CPPFLAGS	=	-I$(top_srcdir)/include -D_LINUX -DTSS_LIB_IS_12
else
AM_CPPFLAGS	=	-I$(top_srcdir)/include -D_LINUX
endif

LDADD		=	$(top_builddir)/lib/libtpm_tspi.la -ltspi $(top_builddir)/lib/libtpm_unseal.la -ltpm_unseal -lcrypto

tpm_sealdata_SOURCES = tpm_sealdata.c
tpm_unsealdata_SOURCES = tpm_unsealdata.c

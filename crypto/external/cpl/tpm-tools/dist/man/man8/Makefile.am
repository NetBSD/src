#
# Makefile.am
#
#       The Initial Developer of the Original Code is International
#       Business Machines Corporation. Portions created by IBM
#       Corporation are Copyright (C) 2005 International Business
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

man8_PODS =

if TSS_LIB_IS_12
man8_PODS +=	tpm_nvdefine.pod	\
		tpm_nvinfo.pod		\
		tpm_nvread.pod		\
		tpm_nvrelease.pod	\
		tpm_nvwrite.pod
endif

man8_MANS =	tpm_changeownerauth.8	\
		tpm_clear.8		\
		tpm_createek.8		\
		tpm_getpubek.8		\
		tpm_restrictpubek.8	\
		tpm_selftest.8		\
		tpm_setactive.8		\
		tpm_setclearable.8	\
		tpm_setenable.8		\
		tpm_setownable.8	\
		tpm_setpresence.8	\
		tpm_takeownership.8	\
		$(addsuffix .8,$(basename $(man8_PODS)))

if TSS_LIB_IS_12
man8_MANS+=tpm_revokeek.8 tpm_setoperatorauth.8 tpm_resetdalock.8
endif

EXTRA_DIST = $(man8_MANS)

%.8 : %.pod
	@pod2man -r "TPM Management" \
		-c "" \
		-n $(basename $@) \
		--section=8 $< > $@

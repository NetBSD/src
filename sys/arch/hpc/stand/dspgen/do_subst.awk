# $NetBSD: do_subst.awk,v 1.1.2.3 2001/03/12 13:28:14 bouyer Exp $
#
# Copyright (c) 1999, 2000 Christopher G. Demetriou.  All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
# 3. All advertising materials mentioning features or use of this software
#    must display the following acknowledgement:
#      This product includes software developed by Christopher G. Demetriou
#      for the NetBSD Project.
# 4. The name of the author may not be used to endorse or promote products
#    derived from this software without specific prior written permission
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
# IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
# OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
# INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
# NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
# THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

function setup_md_files (arch, env, srclist) {
	srclist=""
	asm_tmpl="dspgen/asm_build." arch
	prop_tmpl="dspgen/property." arch

	sz = split(ENVIRON[env], a, "[ \t\n]+");
	for (i = 1; i <= sz; i++) {
		if (a[i] == "") {
			continue
		}
		if (srclist != "") {
			srclist=srclist "\n"
		}
		srclist=srclist "# Begin Source File\n"
		srclist=srclist "\n"
		srclist=srclist "SOURCE=.\\" a[i] "\n"
		base = index (a[i], ".asm")
		if (base != 0) {
		  basename = substr (a[i], 0, base - 1)
		  while (getline < asm_tmpl > 0) {
		    gsub ("%%% ASM_BASENAME %%%", basename)
		    srclist=srclist $0 "\n"
		  }
		  close (asm_tmpl)
		} else {
		  while (getline < prop_tmpl > 0)
		    srclist=srclist $0 "\n"
		    close (prop_tmpl)
		}
		srclist=srclist "# End Source File"
	}
	return srclist
}

BEGIN {
	NAME=ENVIRON["NAME"]

	SRCFILES=""
	sz = split(ENVIRON["SRCFILE_LIST"], a, "[ \t\n]+");
	for (i = 1; i <= sz; i++) {
		if (a[i] == "") {
			continue
		}
		if (SRCFILES != "") {
			SRCFILES=SRCFILES "\n"
		}
		SRCFILES=SRCFILES "# Begin Source File\n"
		SRCFILES=SRCFILES "\n"
		SRCFILES=SRCFILES "SOURCE=.\\" a[i] "\n"
		SRCFILES=SRCFILES "# End Source File"
	}

	SRCFILES_ARM = setup_md_files("ARM", "SRCFILE_LIST_ARM", SRCFILES_ARM)
	SRCFILES_SH3 = setup_md_files("SH3", "SRCFILE_LIST_SH3", SRCFILES_SH3)
	SRCFILES_SH = setup_md_files("SH", "SRCFILE_LIST_SH3", SRCFILES_SH3)
	SRCFILES_MIPS = setup_md_files("MIPS", "SRCFILE_LIST_MIPS",
				       SRCFILES_MIPS)

	CPPDEFS=""
	sz = split(ENVIRON["STD_CPPDEF_LIST"], a, "[ \t\n]+");
	for (i = 1; i <= sz; i++) {
		if (a[i] == "") {
			continue
		}
		if (CPPDEFS != "") {
			CPPDEFS=CPPDEFS " "
		}
		CPPDEFS=CPPDEFS "/D \"" a[i] "\""
	}
	sz = split(ENVIRON["CPPDEF_LIST"], a, "[ \t\n]+");
	for (i = 1; i <= sz; i++) {
		if (a[i] == "") {
			continue
		}
		if (CPPDEFS != "") {
			CPPDEFS=CPPDEFS " "
		}
		CPPDEFS=CPPDEFS "/D \"" a[i] "\""
	}

	INCDIRS=""
	sz = split(ENVIRON["STD_INCDIR_LIST"], a, "[ \t\n]+");
	for (i = 1; i <= sz; i++) {
		if (a[i] == "") {
			continue
		}
		if (INCDIRS != "") {
			INCDIRS=INCDIRS " "
		}
		INCDIRS=INCDIRS "/I \"" a[i] "\""
	}
	sz = split(ENVIRON["INCDIR_LIST"], a, "[ \t\n]+");
	for (i = 1; i <= sz; i++) {
		if (a[i] == "") {
			continue
		}
		if (INCDIRS != "") {
			INCDIRS=INCDIRS " "
		}
		INCDIRS=INCDIRS "/I \"" a[i] "\""
	}
	sz = split(ENVIRON["LIBDEP_LIST"], a, "[ \t\n]+");
	for (i = 1; i <= sz; i++) {
		if (a[i] == "") {
			continue
		}
		if (INCDIRS != "") {
			INCDIRS=INCDIRS " "
		}
		INCDIRS=INCDIRS "/I \"..\\" a[i] "\""
        }

	LIBRARIES=""
	sz = split(ENVIRON["STD_LIBRARY_LIST"], a, "[ \t\n]+");
	for (i = 1; i <= sz; i++) {
		if (a[i] == "") {
			continue
		}
		if (LIBRARIES != "") {
			LIBRARIES=LIBRARIES " "
		}
		LIBRARIES=LIBRARIES a[i] ".lib"
	}
	sz = split(ENVIRON["LIBRARY_LIST"], a, "[ \t\n]+");
	for (i = 1; i <= sz; i++) {
		if (a[i] == "") {
			continue
		}
		if (LIBRARIES != "") {
			LIBRARIES=LIBRARIES " "
		}
		LIBRARIES=LIBRARIES a[i] ".lib"
        }
	sz = split(ENVIRON["LIBDEP_LIST"], a, "[ \t\n]+");
	for (i = 1; i <= sz; i++) {
		if (a[i] == "") {
			continue
		}
		if (LIBRARIES != "") {
			LIBRARIES=LIBRARIES " "
		}
		LIBRARIES=LIBRARIES a[i] ".lib"
	}

	sz = split(ENVIRON["LIBDEP_LIST"], a, "[ \t\n]+");
	DEBUG_LIBPATH=""
	RELEASE_LIBPATH=""
	for (i = 1; i <= sz; i++) {
		if (a[i] == "") {
			continue
		}
		if (i > 1) {
			DEBUG_LIBPATH=DEBUG_LIBPATH " "
			RELEASE_LIBPATH=RELEASE_LIBPATH " "
		}
		DEBUG_LIBPATH=DEBUG_LIBPATH "/libpath:\"..\\" a[i] "\\WMIPSDbg\""
		RELEASE_LIBPATH=RELEASE_LIBPATH "/libpath:\"..\\" a[i] "\\WMIPSRel\""
	}
}
{
	gsub("%%% SRCFILES %%%", SRCFILES)
	gsub("%%% SRCFILES_ARM %%%", SRCFILES_ARM)
	gsub("%%% SRCFILES_SH3 %%%", SRCFILES_SH3)
	gsub("%%% SRCFILES_SH %%%", SRCFILES_SH)
	gsub("%%% SRCFILES_MIPS %%%", SRCFILES_MIPS)
	gsub("%%% CPPDEFS %%%", CPPDEFS)
	gsub("%%% INCDIRS %%%", INCDIRS)
	gsub("%%% LIBRARIES %%%", LIBRARIES)
	gsub("%%% DEBUG_LIBPATH %%%", DEBUG_LIBPATH)
	gsub("%%% RELEASE_LIBPATH %%%", RELEASE_LIBPATH)
	gsub("%%% NAME %%%", NAME)
	print $0
}

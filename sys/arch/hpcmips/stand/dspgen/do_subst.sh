# $NetBSD: do_subst.sh,v 1.4 2001/01/05 15:00:57 takemura Exp $
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

AWK=awk
if [ `uname` = SunOS ]; then
	AWK=nawk
fi

STD_CPPDEF_LIST=''
STD_INCDIR_LIST=''
STD_LIBRARY_LIST=''

export NAME
export SRCFILE_LIST
export CPPDEF_LIST STD_CPPDEF_LIST
export INCDIR_LIST STD_INCDIR_LIST 
export LIBDEP_LIST
export LIBRARY_LIST STD_LIBRARY_LIST

${AWK} '
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
	gsub("%%% NAME %%%", NAME)
	gsub("%%% SRCFILES %%%", SRCFILES)
	gsub("%%% CPPDEFS %%%", CPPDEFS)
	gsub("%%% INCDIRS %%%", INCDIRS)
	gsub("%%% LIBRARIES %%%", LIBRARIES)
	gsub("%%% DEBUG_LIBPATH %%%", DEBUG_LIBPATH)
	gsub("%%% RELEASE_LIBPATH %%%", RELEASE_LIBPATH)
	print $0
}
'

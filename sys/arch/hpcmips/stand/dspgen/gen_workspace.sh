# $NetBSD: gen_workspace.sh,v 1.2 2001/01/05 15:00:57 takemura Exp $
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

vc_ver=$1
shift
WORKSPACE_FILE=$1
shift
SORTED_PROJECTS=`(for project in $*; do
			echo $project
		done) | sort`

(
case $vc_ver in
"vc6")
    echo "Microsoft Developer Studio Workspace File, Format Version 6.00"
    suffix=dsp
    ;;
"evc3")
    echo "Microsoft eMbedded Visual Tools Workspace File, Format Version 3.00"
    suffix=vcp
    ;;
*)
    echo "Unsupported Visual C++ version." 1>&2
    exit 1
esac

cat << __EOF__
# WARNING: DO NOT EDIT OR DELETE THIS WORKSPACE FILE!

###############################################################################
__EOF__

for project in $SORTED_PROJECTS; do
	echo ""
	echo "Project: \"$project\"=.\\$project\\$project.$suffix - Package Owner=<4>"
	echo ""
	echo "Package=<5>"
	echo "{{{"
	echo "}}}"
	echo ""
	echo "Package=<4>"
	echo "{{{"
	for libdep in `( . $project/$project.config; echo $LIBDEP_LIST )`; do
		echo "    Begin Project Dependency"
		echo "    Project_Dep_Name $libdep"
		echo "    End Project Dependency"
	done
	echo "}}}"
	echo ""
	echo "###############################################################################"
done

cat << __EOF__

Global:

Package=<5>
{{{
}}}

Package=<3>
{{{
}}}

###############################################################################

__EOF__
) | awk ' { printf "%s\r\n", $0 }' > ${WORKSPACE_FILE}

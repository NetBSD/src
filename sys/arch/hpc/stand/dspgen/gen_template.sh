# $NetBSD: gen_template.sh,v 1.1.2.3 2001/03/12 13:28:14 bouyer Exp $
#
# Copyright (c) 2001 The NetBSD Foundation, Inc.
# All rights reserved.
#
# This code is derived from software contributed to The NetBSD Foundation
# by UCHIYAMA Yasushi.
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
#        This product includes software developed by the NetBSD
#        Foundation, Inc. and its contributors.
# 4. Neither the name of The NetBSD Foundation nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
# ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
# POSSIBILITY OF SUCH DAMAGE.
#

#
# Generate application/static library dsp/vcp template.
#
TEMPLATE=$TYPE.tmpl
DEBUG_VER="Debug"
RELEASE_VER="Release"
if [ $TYPE != "application" ] ; then
    PROJECTTYPE="Static Library"
else
    PROJECTTYPE="Application"
fi

#
# Visual C++ version.
#
vc_ver=$1
case $vc_ver in
"vc5")
    echo "Generate template for Visual C++ 5 Windows CE Embeded ToolKit"
    ;;
"vc6")
    echo "Generate template for Visual C++ 6 Windows CE ToolKit"
    ;;
"evc3")
    echo "Generate template for Embeded Visual C++ 3"
    ;;
*)
    echo "Unsupported Visual C++ version."
    exit 1
esac

tmpl_base="tmpl.$vc_ver"
header_tmpl="$tmpl_base/header"
project_tmpl="$tmpl_base/project"

#
# Architecture types.
#
archs=""
i=1
n=`expr $#`
while [ $i -lt $n ]; do
    shift 1
    archs="$archs $1"
    i=`expr $i + 1`
done
echo "Supported architecture [$archs ]"

#
# DSP/VCP header.
#
cat $header_tmpl >> $TEMPLATE 

#
# Configuration declare.
#
echo '!MESSAGE' >> $TEMPLATE
for arch in $archs
do
    ARCH_CFG="Win32 (WCE $arch)"
    CFG="%%% NAME %%% - $ARCH_CFG"
    echo "!MESSAGE \"$CFG $DEBUG_VER\" (\"$ARCH_CFG $PROJECTTYPE\")" >> $TEMPLATE
    echo "!MESSAGE \"$CFG $RELEASE_VER\" (\"$ARCH_CFG $PROJECTTYPE\")" >> $TEMPLATE
done
echo '!MESSAGE' >> $TEMPLATE

#
# Project define.
#
cat $project_tmpl >> $TEMPLATE

#
# Architecture dependent configuration define.
#
for arch in $archs
do
    ARCH_CFG="Win32 (WCE $arch)"
    CFG="%%% NAME %%% - $ARCH_CFG"
    echo "!IF  \"\$(CFG)\" == \"$CFG $DEBUG_VER\"" >> $TEMPLATE
    echo "# PROP BASE Output_Dir \"..\\compile\\$arch$DEBUG_VER\"" >> $TEMPLATE
    echo "# PROP BASE Intermediate_Dir \"..\\compile\\$arch$DEBUG_VER\"" >> $TEMPLATE
    echo "# PROP Output_Dir \"..\\compile\\$arch$DEBUG_VER\"" >> $TEMPLATE
    echo "# PROP Intermediate_Dir \"..\\compile\\$arch$DEBUG_VER\"" >> $TEMPLATE
    cat $tmpl_base/$TYPE.$arch$DEBUG_VER >> $TEMPLATE
    cat $tmpl_base/config.defadd >> $TEMPLATE
    if [ $TYPE = "application" ] ; then
	echo "# ADD LINK32 /libpath:\"..\\compile\\$arch$DEBUG_VER\"" >> $TEMPLATE
    fi
    echo "!ENDIF" >> $TEMPLATE
    echo "!IF  \"\$(CFG)\" == \"$CFG $RELEASE_VER\"" >> $TEMPLATE
    cat $tmpl_base/config.defprop >> $TEMPLATE
    echo "# PROP BASE Output_Dir \"..\\compile\\$arch$RELEASE_VER\"" >> $TEMPLATE
    echo "# PROP BASE Intermediate_Dir \"..\\compile\\$arch$RELEASE_VER\"" >> $TEMPLATE
    echo "# PROP Output_Dir \"..\\compile\\$arch$RELEASE_VER\"" >> $TEMPLATE
    echo "# PROP Intermediate_Dir \"..\\compile\\$arch$RELEASE_VER\"" >> $TEMPLATE
    cat $tmpl_base/$TYPE.$arch$RELEASE_VER >> $TEMPLATE
    cat $tmpl_base/config.defadd >> $TEMPLATE
    if [ $TYPE = "application" ] ; then
	echo "# ADD LINK32 /libpath:\"..\\compile\\$arch$RELEASE_VER\"" >> $TEMPLATE
    fi
    echo "!ENDIF" >> $TEMPLATE
done

#
# Target declare
#
echo >> $TEMPLATE
echo "# Begin Target" >> $TEMPLATE
for arch in $archs
do
    ARCH_CFG="Win32 (WCE $arch)"
    CFG="%%% NAME %%% - $ARCH_CFG"
    echo "# Name \"$CFG $DEBUG_VER\"" >> $TEMPLATE
    echo "# Name \"$CFG $RELEASE_VER\"" >> $TEMPLATE
done

#
# Source files
#
# (MI)
cat >> $TEMPLATE <<SRCS

# Begin Group "Source Files"
%%% SRCFILES %%%

SRCS
# (MD)
for arch in $archs
do
    echo "%%% SRCFILES_$arch %%%" >> $TEMPLATE
done
#
# Footer.
#
cat >> $TEMPLATE <<FOOTER

# End Group
# End Target
# End Project

FOOTER

#
# Generate MD source files property.
#
for arch in $archs
do
    ARCH_FILE="property.$arch"
    rm -f $ARCH_FILE
    for sub_arch in $archs
    do
	ARCH_CFG="Win32 (WCE $sub_arch)"
	CFG="%%% NAME %%% - $ARCH_CFG"
	CONDITION_DEBUG="!IF  \"\$(CFG)\" == \"$CFG $DEBUG_VER\""
	CONDITION_RELEASE="!IF  \"\$(CFG)\" == \"$CFG $RELEASE_VER\""
	if [ $sub_arch != $arch ] ; then
	    echo $CONDITION_DEBUG >> $ARCH_FILE >> $ARCH_FILE
	    echo "# PROP Exclude_From_Build 1" >> $ARCH_FILE
	    echo "!ENDIF" >> $ARCH_FILE
	    echo $CONDITION_RELEASE >> $ARCH_FILE >> $ARCH_FILE
	    echo "# PROP Exclude_From_Build 1" >> $ARCH_FILE
	    echo "!ENDIF" >> $ARCH_FILE
	fi
    done
#    mv $ARCH_FILE $ARCH_FILE.0
#    awk ' { printf "%s\r\n", $0 }' $ARCH_FILE.0 > $ARCH_FILE
#    rm -f $ARCH_FILE.0
done

#
# Generate MD assembler files property and custom build method.
#
for arch in $archs
do
    ARCH_FILE="asm_build.$arch.0"
    rm -f $ARCH_FILE
    for sub_arch in $archs
    do
	ARCH_CFG="Win32 (WCE $sub_arch)"
	CFG="%%% NAME %%% - $ARCH_CFG"
	CONDITION_DEBUG="!IF  \"\$(CFG)\" == \"$CFG $DEBUG_VER\""
	CONDITION_RELEASE="!IF  \"\$(CFG)\" == \"$CFG $RELEASE_VER\""
	if [ $sub_arch != $arch ] ; then
	    echo $CONDITION_DEBUG >> $ARCH_FILE >> $ARCH_FILE
	    echo "# PROP Exclude_From_Build 1" >> $ARCH_FILE
	    echo "!ENDIF" >> $ARCH_FILE
	    echo $CONDITION_RELEASE >> $ARCH_FILE >> $ARCH_FILE
	    echo "# PROP Exclude_From_Build 1" >> $ARCH_FILE
	    echo "!ENDIF" >> $ARCH_FILE
	else
	    echo $CONDITION_DEBUG >> $ARCH_FILE >> $ARCH_FILE
	    echo "# PROP Ignore_Default_Tool 1" >> $ARCH_FILE
	    echo "# Begin Custom Build" >> $ARCH_FILE
	    echo "InputPath=%%% ASM_BASENAME %%%.asm" >> $ARCH_FILE
	    echo "\"\$(OUTDIR)\\%%% ASM_BASENAME %%%.obj\" : \$(SOURCE) \"\$(INTDIR)\" \"\$(OUTDIR)\"" >> $ARCH_FILE
	    echo "	%%% ASM %%% \$(InputPath) \$(OUTDIR)\\%%% ASM_BASENAME %%%.obj" >> $ARCH_FILE
	    echo "# End Custom Build" >> $ARCH_FILE
	    echo "!ENDIF" >> $ARCH_FILE
	    echo $CONDITION_RELEASE >> $ARCH_FILE >> $ARCH_FILE
	    echo "# PROP Ignore_Default_Tool 1" >> $ARCH_FILE
	    echo "# Begin Custom Build" >> $ARCH_FILE
	    echo "InputPath=%%% ASM_BASENAME %%%.asm" >> $ARCH_FILE
	    echo "\"\$(OUTDIR)\\%%% ASM_BASENAME %%%.obj\" : \$(SOURCE) \"\$(INTDIR)\" \"\$(OUTDIR)\"" >> $ARCH_FILE
	    echo "	%%% ASM %%% \$(InputPath) \$(OUTDIR)\\%%% ASM_BASENAME %%%.obj" >> $ARCH_FILE
	    echo "# End Custom Build" >> $ARCH_FILE
	    echo "!ENDIF" >> $ARCH_FILE
	fi
    done
done

#
# set assembler.
#
if [ -f asm_build.ARM.0 ]; then
    sed 's/%%% ASM %%%/armasm.exe/' asm_build.ARM.0 > asm_build.ARM
    rm -f asm_build.ARM.0
fi
if [ -f asm_build.SH.0 ]; then
    sed 's/%%% ASM %%%/asmsh.exe/' asm_build.SH.0 > asm_build.SH
    rm -f asm_build.SH.0
fi
if [ -f asm_build.SH3.0 ]; then
    sed 's/%%% ASM %%%/asmsh.exe/' asm_build.SH3.0 > asm_build.SH3
    rm -f asm_build.SH3.0
fi
if [ -f asm_build.SH4.0 ]; then
    sed 's/%%% ASM %%%/asmsh.exe/' asm_build.SH4.0 > asm_build.SH4
    rm -f asm_build.SH4.0
fi
if [ -f asm_build.MIPS.0 ]; then
    sed 's/%%% ASM %%%/mipsasm.exe/' asm_build.MIPS.0 > asm_build.MIPS
    rm -f asm_build.MIPS.0
fi

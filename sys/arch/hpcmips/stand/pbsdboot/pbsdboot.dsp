# Microsoft Developer Studio Project File - Name="pbsdboot" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** 編集しないでください **

# TARGTYPE "Win32 (WCE MIPS) Application" 0x0a01

CFG=pbsdboot - Win32 (WCE MIPS) Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "pbsdboot.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "pbsdboot.mak" CFG="pbsdboot - Win32 (WCE MIPS) Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "pbsdboot - Win32 (WCE MIPS) Release" ("Win32 (WCE MIPS) Application"\
 用)
!MESSAGE "pbsdboot - Win32 (WCE MIPS) Debug" ("Win32 (WCE MIPS) Application" 用)
!MESSAGE 

# Begin Project
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
# PROP WCE_Configuration "H/PC Ver. 2.00"
CPP=clmips.exe
MTL=midl.exe
RSC=rc.exe

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WMIPSRel"
# PROP BASE Intermediate_Dir "WMIPSRel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 1
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WMIPSRel"
# PROP Intermediate_Dir "WMIPSRel"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MT /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /Yu"stdafx.h" /QMRWCE /c
# ADD CPP /nologo /MT /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /Yu"stdafx.h" /QMRWCE /c
# ADD BASE RSC /l 0x411 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x411 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /NOLOGO
# ADD BSC32 /NOLOGO
LINK32=link.exe
# ADD BASE LINK32 /nologo /entry:"wWinMainCRTStartup" /machine:MIPS /subsystem:$(CESubsystem)
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 /nologo /entry:"wWinMainCRTStartup" /machine:MIPS /subsystem:$(CESubsystem)
# SUBTRACT LINK32 /pdb:none /nodefaultlib
PFILE=pfile.exe
# ADD BASE PFILE COPY
# ADD PFILE COPY

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

# PROP BASE Use_MFC 1
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WMIPSDbg"
# PROP BASE Intermediate_Dir "WMIPSDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WMIPSDbg"
# PROP Intermediate_Dir "WMIPSDbg"
# PROP Ignore_Export_Lib 0
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MTd /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /Yu"stdafx.h" /QMRWCE /c
# ADD CPP /nologo /MLd /W3 /Zi /Od /I "." /I "../libsa" /I "../include" /I "../../../.." /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /QMRWCE /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x411 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x411 /r /d "MIPS" /d "_MIPS_" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /NOLOGO
# ADD BSC32 /NOLOGO
LINK32=link.exe
# ADD BASE LINK32 /nologo /entry:"wWinMainCRTStartup" /debug /machine:MIPS /subsystem:$(CESubsystem)
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib libsa.lib winsock.lib /nologo /incremental:no /debug /machine:MIPS /libpath:"../libsa/WMIPSDbg" /subsystem:$(CESubsystem)
# SUBTRACT LINK32 /verbose /profile /pdb:none /map /nodefaultlib
PFILE=pfile.exe
# ADD BASE PFILE COPY
# ADD PFILE COPY

!ENDIF 

# Begin Target

# Name "pbsdboot - Win32 (WCE MIPS) Release"
# Name "pbsdboot - Win32 (WCE MIPS) Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\disptest.c

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

DEP_CPP_DISPT=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\errno.h"\
	"..\..\..\..\sys\featuretest.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\include\ansi.h"\
	"..\..\include\bootinfo.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\..\include\platid.h"\
	"..\..\include\platid_generated.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bootinfo.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\platid.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	"..\libsa\compat.h"\
	"..\libsa\stand.h"\
	".\pbsdboot.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\elf.c

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

DEP_CPP_ELF_C=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\errno.h"\
	"..\..\..\..\sys\exec_elf.h"\
	"..\..\..\..\sys\featuretest.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\elf_machdep.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\include\ansi.h"\
	"..\..\include\bootinfo.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\elf_machdep.h"\
	"..\..\include\endian.h"\
	"..\..\include\platid.h"\
	"..\..\include\platid_generated.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bootinfo.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\elf_machdep.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\platid.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\elf_machdep.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	"..\libsa\compat.h"\
	"..\libsa\stand.h"\
	".\pbsdboot.h"\
	
NODEP_CPP_ELF_C=\
	"..\..\..\..\sys\opt_execfmt.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\layout.c

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

DEP_CPP_LAYOU=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\errno.h"\
	"..\..\..\..\sys\featuretest.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\include\ansi.h"\
	"..\..\include\bootinfo.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\..\include\platid.h"\
	"..\..\include\platid_generated.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bootinfo.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\platid.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	"..\libsa\compat.h"\
	"..\libsa\stand.h"\
	".\pbsdboot.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\main.c

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

DEP_CPP_MAIN_=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\errno.h"\
	"..\..\..\..\sys\featuretest.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\include\ansi.h"\
	"..\..\include\bootinfo.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\..\include\platid.h"\
	"..\..\include\platid_generated.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bootinfo.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\platid.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	"..\libsa\compat.h"\
	"..\libsa\stand.h"\
	".\pbsdboot.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mips.c

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

DEP_CPP_MIPS_=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\errno.h"\
	"..\..\..\..\sys\featuretest.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\include\ansi.h"\
	"..\..\include\bootinfo.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\..\include\platid.h"\
	"..\..\include\platid_generated.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bootinfo.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\platid.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	"..\libsa\compat.h"\
	"..\libsa\stand.h"\
	".\pbsdboot.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\pbsdboot.c

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

DEP_CPP_PBSDB=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\errno.h"\
	"..\..\..\..\sys\featuretest.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\include\ansi.h"\
	"..\..\include\bootinfo.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\..\include\platid.h"\
	"..\..\include\platid_generated.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bootinfo.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\platid.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	"..\libsa\compat.h"\
	"..\libsa\stand.h"\
	".\pbsdboot.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\res\pbsdboot.rc

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\platid.c

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

DEP_CPP_PLATI=\
	"..\..\..\..\lib\libkern\libkern.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\systm.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\hpcmips\platid.c"\
	"..\..\include\ansi.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\..\include\platid.h"\
	"..\..\include\platid_generated.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\platid.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	"..\libsa\compat.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\platid_mask.c

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

DEP_CPP_PLATID=\
	"..\..\hpcmips\platid_mask.c"\
	"..\..\include\platid.h"\
	"..\..\include\platid_generated.h"\
	"..\include\machine\platid.h"\
	"..\libsa\compat.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\preference.c

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

DEP_CPP_PREFE=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\errno.h"\
	"..\..\..\..\sys\featuretest.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\include\ansi.h"\
	"..\..\include\bootinfo.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\..\include\platid.h"\
	"..\..\include\platid_generated.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bootinfo.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\platid.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	"..\libsa\compat.h"\
	"..\libsa\stand.h"\
	".\pbsdboot.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\print.c

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

DEP_CPP_PRINT=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\errno.h"\
	"..\..\..\..\sys\featuretest.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\include\ansi.h"\
	"..\..\include\bootinfo.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\..\include\platid.h"\
	"..\..\include\platid_generated.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bootinfo.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\platid.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	"..\libsa\compat.h"\
	"..\libsa\stand.h"\
	".\pbsdboot.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\systeminfo.c

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

DEP_CPP_SYSTE=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\errno.h"\
	"..\..\..\..\sys\featuretest.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\include\ansi.h"\
	"..\..\include\bootinfo.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\..\include\platid.h"\
	"..\..\include\platid_generated.h"\
	"..\..\include\platid_mask.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bootinfo.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\platid.h"\
	"..\include\machine\platid_mask.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	"..\libsa\compat.h"\
	"..\libsa\stand.h"\
	".\pbsdboot.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\tx39xx.c

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

DEP_CPP_TX39X=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\errno.h"\
	"..\..\..\..\sys\featuretest.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\include\ansi.h"\
	"..\..\include\bootinfo.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\..\include\platid.h"\
	"..\..\include\platid_generated.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bootinfo.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\platid.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	"..\libsa\compat.h"\
	"..\libsa\stand.h"\
	".\pbsdboot.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vmem.c

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

DEP_CPP_VMEM_=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\errno.h"\
	"..\..\..\..\sys\featuretest.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\include\ansi.h"\
	"..\..\include\bootinfo.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\..\include\platid.h"\
	"..\..\include\platid_generated.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bootinfo.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\platid.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	"..\libsa\compat.h"\
	"..\libsa\stand.h"\
	".\pbsdboot.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\vr41xx.c

!IF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "pbsdboot - Win32 (WCE MIPS) Debug"

DEP_CPP_VR41X=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\errno.h"\
	"..\..\..\..\sys\featuretest.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\include\ansi.h"\
	"..\..\include\bootinfo.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\..\include\platid.h"\
	"..\..\include\platid_generated.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bootinfo.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\platid.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	"..\libsa\compat.h"\
	"..\libsa\stand.h"\
	".\pbsdboot.h"\
	

!ENDIF 

# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\bootinfo.h
# End Source File
# End Group
# Begin Group "Resource Files"

# PROP Default_Filter "ico;cur;bmp;dlg;rc2;rct;bin;cnt;rtf;gif;jpg;jpeg;jpe"
# Begin Source File

SOURCE=.\res\pbsd.bmp
# End Source File
# Begin Source File

SOURCE=.\res\pbsd.ico
# End Source File
# End Group
# End Target
# End Project

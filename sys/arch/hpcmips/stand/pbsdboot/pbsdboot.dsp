# Microsoft Developer Studio Project File - Name="pbsdboot" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE MIPS) Application" 0x0a01

CFG=pbsdboot - Win32 (WCE MIPS) Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "pbsdboot.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "pbsdboot.mak" CFG="pbsdboot - Win32 (WCE MIPS) Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "pbsdboot - Win32 (WCE MIPS) Release" (based on "Win32 (WCE MIPS) Application")
!MESSAGE "pbsdboot - Win32 (WCE MIPS) Debug" (based on "Win32 (WCE MIPS) Application")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
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
# ADD BASE CPP /nologo /MT /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D "hpcmips" /D UNDER_CE=$(CEVersion) /D "UNICODE" /Yu"stdafx.h" /QMRWCE /c
# ADD CPP /nologo /MT /W3 /O2 /I "." /I "../../../../sys" /I "../../../.." /I "../include" /I "..\libsa" /I "..\libz" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D "hpcmips" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D _STANDALONE /D __STDC__ /D __signed=signed /D LIBSA_RENAME_PRINTF /D __COMPILER_INT64__=__int64 /D __COMPILER_UINT64__="unsigned __int64" /QMRWCE /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x411 /r /d "MIPS" /d "_MIPS_" /D "hpcmips" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
# ADD RSC /l 0x411 /r /d "MIPS" /d "_MIPS_" /D "hpcmips" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "NDEBUG"
# ADD BASE MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "NDEBUG" /mktyplib203 /o NUL /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /NOLOGO
# ADD BSC32 /NOLOGO
LINK32=link.exe
# ADD BASE LINK32 /nologo /entry:"wWinMainCRTStartup" /machine:MIPS /subsystem:$(CESubsystem)
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib winsock.lib libsa.lib libz.lib /nologo /incremental:no /machine:MIPS /subsystem:$(CESubsystem)  /libpath:"..\libsa\WMIPSRel" /libpath:"..\libz\WMIPSRel"
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
# ADD BASE CPP /nologo /MTd /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D "hpcmips" /D UNDER_CE=$(CEVersion) /D "UNICODE" /Yu"stdafx.h" /QMRWCE /c
# ADD CPP /nologo /MLd /W3 /Zi /Od /I "." /I "../../../../sys" /I "../../../.." /I "../include" /I "..\libsa" /I "..\libz" /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D "hpcmips" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D _STANDALONE /D __STDC__ /D __signed=signed /D LIBSA_RENAME_PRINTF /D __COMPILER_INT64__=__int64 /D __COMPILER_UINT64__="unsigned __int64" /QMRWCE /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x411 /r /d "MIPS" /d "_MIPS_" /D "hpcmips" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD RSC /l 0x411 /r /d "MIPS" /d "_MIPS_" /D "hpcmips" /d UNDER_CE=$(CEVersion) /d _WIN32_WCE=$(CEVersion) /d "$(CEConfigName)" /d "UNICODE" /d "DEBUG"
# ADD BASE MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
# ADD MTL /nologo /D "_DEBUG" /mktyplib203 /o NUL /win32
BSC32=bscmake.exe
# ADD BASE BSC32 /NOLOGO
# ADD BSC32 /NOLOGO
LINK32=link.exe
# ADD BASE LINK32 /nologo /entry:"wWinMainCRTStartup" /debug /machine:MIPS /subsystem:$(CESubsystem)
# SUBTRACT BASE LINK32 /pdb:none /nodefaultlib
# ADD LINK32 commctrl.lib coredll.lib winsock.lib libsa.lib libz.lib /nologo /incremental:no /debug /machine:MIPS /subsystem:$(CESubsystem)  /libpath:"..\libsa\WMIPSDbg" /libpath:"..\libz\WMIPSDbg"
# SUBTRACT LINK32 /verbose /profile /pdb:none /map /nodefaultlib
PFILE=pfile.exe
# ADD BASE PFILE COPY
# ADD PFILE COPY

!ENDIF 

# Begin Target

# Name "pbsdboot - Win32 (WCE MIPS) Release"
# Name "pbsdboot - Win32 (WCE MIPS) Debug"
# Begin Source File

SOURCE=.\disptest.c
# End Source File
# Begin Source File

SOURCE=.\elf.c
# End Source File
# Begin Source File

SOURCE=.\hpccmap.c
# End Source File
# Begin Source File

SOURCE=.\layout.c
# End Source File
# Begin Source File

SOURCE=.\main.c
# End Source File
# Begin Source File

SOURCE=.\mips.c
# End Source File
# Begin Source File

SOURCE=.\palette.c
# End Source File
# Begin Source File

SOURCE=.\pbsdboot.c
# End Source File
# Begin Source File

SOURCE=.\platid.c
# End Source File
# Begin Source File

SOURCE=.\platid_mask.c
# End Source File
# Begin Source File

SOURCE=.\platid_name.c
# End Source File
# Begin Source File

SOURCE=.\preference.c
# End Source File
# Begin Source File

SOURCE=.\print.c
# End Source File
# Begin Source File

SOURCE=.\res/pbsd.bmp
# End Source File
# Begin Source File

SOURCE=.\res/pbsd.ico
# End Source File
# Begin Source File

SOURCE=.\res/pbsdboot.rc
# End Source File
# Begin Source File

SOURCE=.\systeminfo.c
# End Source File
# Begin Source File

SOURCE=.\tx39xx.c
# End Source File
# Begin Source File

SOURCE=.\vmem.c
# End Source File
# Begin Source File

SOURCE=.\vr41xx.c
# End Source File
# End Target
# End Project

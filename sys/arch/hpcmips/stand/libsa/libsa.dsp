# Microsoft Developer Studio Project File - Name="libsa" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (WCE MIPS) Static Library" 0x0a04

CFG=libsa - Win32 (WCE MIPS) Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libsa.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libsa.mak" CFG="libsa - Win32 (WCE MIPS) Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libsa - Win32 (WCE MIPS) Release" (based on "Win32 (WCE MIPS) Static Library")
!MESSAGE "libsa - Win32 (WCE MIPS) Debug" (based on "Win32 (WCE MIPS) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
# PROP WCE_Configuration "H/PC Ver. 2.00"
CPP=clmips.exe

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WMIPSRel"
# PROP BASE Intermediate_Dir "WMIPSRel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WMIPSRel"
# PROP Intermediate_Dir "WMIPSRel"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /ML /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /YX /QMRWCE /c
# ADD CPP /nologo /ML /W3 /O2 /I "." /I "../include" /I "../../../.." /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_STANDALONE" /QMRWCE /c
# SUBTRACT CPP /YX
BSC32=bscmake.exe
# ADD BASE BSC32 /NOLOGO
# ADD BSC32 /NOLOGO
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WMIPSDbg"
# PROP BASE Intermediate_Dir "WMIPSDbg"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WMIPSDbg"
# PROP Intermediate_Dir "WMIPSDbg"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /MLd /W3 /Zi /Od /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /YX /QMRWCE /c
# ADD CPP /nologo /MLd /W3 /Zi /Od /I "." /I "../include" /I "../../../.." /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /D "_STANDALONE" /QMRWCE /c
# SUBTRACT CPP /YX
BSC32=bscmake.exe
# ADD BASE BSC32 /NOLOGO
# ADD BSC32 /NOLOGO
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libsa - Win32 (WCE MIPS) Release"
# Name "libsa - Win32 (WCE MIPS) Debug"
# Begin Source File

SOURCE=.\alloc.c
# End Source File
# Begin Source File

SOURCE=.\bcopy.c
# End Source File
# Begin Source File

SOURCE=.\close.c
# End Source File
# Begin Source File

SOURCE=.\devopen.c
# End Source File
# Begin Source File

SOURCE=.\dkcksum.c
# End Source File
# Begin Source File

SOURCE=.\lseek.c
# End Source File
# Begin Source File

SOURCE=.\open.c
# End Source File
# Begin Source File

SOURCE=.\printf.c
# End Source File
# Begin Source File

SOURCE=.\read.c
# End Source File
# Begin Source File

SOURCE=.\subr_prf.c
# End Source File
# Begin Source File

SOURCE=.\twiddle.c
# End Source File
# Begin Source File

SOURCE=.\ufs.c
# End Source File
# Begin Source File

SOURCE=.\winblk.c
# End Source File
# Begin Source File

SOURCE=.\winfs.c
# End Source File
# End Target
# End Project

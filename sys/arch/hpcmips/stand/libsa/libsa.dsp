# Microsoft Developer Studio Project File - Name="libsa" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 5.00
# ** 編集しないでください **

# TARGTYPE "Win32 (WCE MIPS) Static Library" 0x0a04

CFG=libsa - Win32 (WCE MIPS) Debug
!MESSAGE これは有効なﾒｲｸﾌｧｲﾙではありません。 このﾌﾟﾛｼﾞｪｸﾄをﾋﾞﾙﾄﾞするためには NMAKE を使用してください。
!MESSAGE [ﾒｲｸﾌｧｲﾙのｴｸｽﾎﾟｰﾄ] ｺﾏﾝﾄﾞを使用して実行してください
!MESSAGE 
!MESSAGE NMAKE /f "libsa.mak".
!MESSAGE 
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "libsa.mak" CFG="libsa - Win32 (WCE MIPS) Debug"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "libsa - Win32 (WCE MIPS) Release" ("Win32 (WCE MIPS) Static Library"\
 用)
!MESSAGE "libsa - Win32 (WCE MIPS) Debug" ("Win32 (WCE MIPS) Static Library" 用)
!MESSAGE 

# Begin Project
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
# ADD CPP /nologo /ML /W3 /O2 /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "NDEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /YX /QMRWCE /c
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
# ADD CPP /nologo /MLd /W3 /Zi /Od /I "." /I "../include" /I "../../../.." /D _WIN32_WCE=$(CEVersion) /D "$(CEConfigName)" /D "DEBUG" /D "MIPS" /D "_MIPS_" /D UNDER_CE=$(CEVersion) /D "UNICODE" /QMRWCE /c
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

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

DEP_CPP_ALLOC=\
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
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	".\compat.h"\
	".\stand.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\bcopy.c

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

DEP_CPP_BCOPY=\
	"..\..\..\..\lib\libsa\bcopy.c"\
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
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	".\compat.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\close.c

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

DEP_CPP_CLOSE=\
	"..\..\..\..\lib\libsa\close.c"\
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
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	".\compat.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\devopen.c

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

DEP_CPP_DEVOP=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\lib\libsa\ufs.h"\
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
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	".\compat.h"\
	".\stand.h"\
	".\winblk.h"\
	".\winfs.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\dkcksum.c

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

DEP_CPP_DKCKS=\
	"..\..\..\..\lib\libsa\dkcksum.c"\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\disklabel.h"\
	"..\..\..\..\sys\dkbad.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\errno.h"\
	"..\..\..\..\sys\featuretest.h"\
	"..\..\..\..\sys\inttypes.h"\
	"..\..\..\..\sys\param.h"\
	"..\..\..\..\sys\resource.h"\
	"..\..\..\..\sys\signal.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\syslimits.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\..\sys\ucred.h"\
	"..\..\..\..\sys\uio.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\..\mips\include\limits.h"\
	"..\..\..\mips\include\mips_param.h"\
	"..\..\..\mips\include\signal.h"\
	"..\..\include\ansi.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\disklabel.h"\
	"..\..\include\endian.h"\
	"..\..\include\intr.h"\
	"..\..\include\limits.h"\
	"..\..\include\param.h"\
	"..\..\include\signal.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\disklabel.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\intr.h"\
	"..\include\machine\limits.h"\
	"..\include\machine\param.h"\
	"..\include\machine\signal.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\limits.h"\
	"..\include\mips\mips_param.h"\
	"..\include\mips\signal.h"\
	"..\include\mips\types.h"\
	".\compat.h"\
	
NODEP_CPP_DKCKS=\
	"..\..\include\opt_gateway.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\lseek.c

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

DEP_CPP_LSEEK=\
	"..\..\..\..\lib\libsa\lseek.c"\
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
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	".\compat.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\open.c

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

DEP_CPP_OPEN_=\
	"..\..\..\..\lib\libsa\open.c"\
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
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	".\compat.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\printf.c

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

DEP_CPP_PRINT=\
	"..\..\..\..\lib\libsa\printf.c"\
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
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\stdarg.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	".\compat.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\read.c

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

DEP_CPP_READ_=\
	"..\..\..\..\lib\libsa\read.c"\
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
	"..\..\..\..\sys\inttypes.h"\
	"..\..\..\..\sys\param.h"\
	"..\..\..\..\sys\resource.h"\
	"..\..\..\..\sys\signal.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\syslimits.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\..\sys\ucred.h"\
	"..\..\..\..\sys\uio.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\..\mips\include\limits.h"\
	"..\..\..\mips\include\mips_param.h"\
	"..\..\..\mips\include\signal.h"\
	"..\..\include\ansi.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\..\include\intr.h"\
	"..\..\include\limits.h"\
	"..\..\include\param.h"\
	"..\..\include\signal.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\intr.h"\
	"..\include\machine\limits.h"\
	"..\include\machine\param.h"\
	"..\include\machine\signal.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\limits.h"\
	"..\include\mips\mips_param.h"\
	"..\include\mips\signal.h"\
	"..\include\mips\types.h"\
	".\compat.h"\
	
NODEP_CPP_READ_=\
	"..\..\include\opt_gateway.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\subr_prf.c

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

DEP_CPP_SUBR_=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\lib\libsa\subr_prf.c"\
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
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\stdarg.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	".\compat.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\twiddle.c

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

DEP_CPP_TWIDD=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\lib\libsa\twiddle.c"\
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
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	".\compat.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\ufs.c

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

DEP_CPP_UFS_C=\
	"..\..\..\..\lib\libkern\libkern.h"\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\lib\libsa\ufs.c"\
	"..\..\..\..\lib\libsa\ufs.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\endian.h"\
	"..\..\..\..\sys\errno.h"\
	"..\..\..\..\sys\featuretest.h"\
	"..\..\..\..\sys\inttypes.h"\
	"..\..\..\..\sys\param.h"\
	"..\..\..\..\sys\resource.h"\
	"..\..\..\..\sys\signal.h"\
	"..\..\..\..\sys\stat.h"\
	"..\..\..\..\sys\syslimits.h"\
	"..\..\..\..\sys\time.h"\
	"..\..\..\..\sys\types.h"\
	"..\..\..\..\sys\ucred.h"\
	"..\..\..\..\sys\uio.h"\
	"..\..\..\..\ufs\ffs\fs.h"\
	"..\..\..\..\ufs\ufs\dinode.h"\
	"..\..\..\..\ufs\ufs\dir.h"\
	"..\..\..\mips\include\ansi.h"\
	"..\..\..\mips\include\bswap.h"\
	"..\..\..\mips\include\cdefs.h"\
	"..\..\..\mips\include\endian.h"\
	"..\..\..\mips\include\limits.h"\
	"..\..\..\mips\include\mips_param.h"\
	"..\..\..\mips\include\signal.h"\
	"..\..\include\ansi.h"\
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\..\include\intr.h"\
	"..\..\include\limits.h"\
	"..\..\include\param.h"\
	"..\..\include\signal.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\intr.h"\
	"..\include\machine\limits.h"\
	"..\include\machine\param.h"\
	"..\include\machine\signal.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\limits.h"\
	"..\include\mips\mips_param.h"\
	"..\include\mips\signal.h"\
	"..\include\mips\types.h"\
	".\compat.h"\
	
NODEP_CPP_UFS_C=\
	"..\..\include\opt_gateway.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\winblk.c

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

DEP_CPP_WINBL=\
	"..\..\..\..\lib\libsa\saerrno.h"\
	"..\..\..\..\lib\libsa\saioctl.h"\
	"..\..\..\..\lib\libsa\stand.h"\
	"..\..\..\..\sys\bswap.h"\
	"..\..\..\..\sys\cdefs.h"\
	"..\..\..\..\sys\cdefs_aout.h"\
	"..\..\..\..\sys\cdefs_elf.h"\
	"..\..\..\..\sys\disklabel.h"\
	"..\..\..\..\sys\dkbad.h"\
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
	"..\..\include\cdefs.h"\
	"..\..\include\disklabel.h"\
	"..\..\include\endian.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\disklabel.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	".\compat.h"\
	".\stand.h"\
	".\winblk.h"\
	{$(INCLUDE)}"diskio.h"\
	

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\winfs.c

!IF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Release"

!ELSEIF  "$(CFG)" == "libsa - Win32 (WCE MIPS) Debug"

DEP_CPP_WINFS=\
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
	"..\..\include\cdefs.h"\
	"..\..\include\endian.h"\
	"..\include\machine\ansi.h"\
	"..\include\machine\bswap.h"\
	"..\include\machine\cdefs.h"\
	"..\include\machine\endian.h"\
	"..\include\machine\types.h"\
	"..\include\mips\ansi.h"\
	"..\include\mips\cdefs.h"\
	"..\include\mips\endian.h"\
	"..\include\mips\types.h"\
	".\compat.h"\
	".\stand.h"\
	".\winfs.h"\
	

!ENDIF 

# End Source File
# End Target
# End Project

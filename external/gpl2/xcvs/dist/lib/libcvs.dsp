# Microsoft Developer Studio Project File - Name="libcvs" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libcvs - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libcvs.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libcvs.mak" CFG="libcvs - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libcvs - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libcvs - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libcvs - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "WinRel"
# PROP BASE Intermediate_Dir "WinRel"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "WinRel"
# PROP Intermediate_Dir "WinRel"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /W3 /GX /O2 /I "..\windows-NT" /I "." /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /YX /FD /c
# ADD BASE RSC /l 0x409 /d "NDEBUG"
# ADD RSC /l 0x409 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ELSEIF  "$(CFG)" == "libcvs - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "WinDebug"
# PROP BASE Intermediate_Dir "WinDebug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "WinDebug"
# PROP Intermediate_Dir "WinDebug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ /c
# ADD CPP /nologo /W3 /Gm /GX /ZI /Od /I "..\windows-NT" /I "." /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /YX /FD /GZ /c
# ADD BASE RSC /l 0x409 /d "_DEBUG"
# ADD RSC /l 0x409 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo

!ENDIF 

# Begin Target

# Name "libcvs - Win32 Release"
# Name "libcvs - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\__fpending.c
# End Source File
# Begin Source File

SOURCE=.\asnprintf.c
# End Source File
# Begin Source File

SOURCE=.\basename.c
# End Source File
# Begin Source File

SOURCE=".\canon-host.c"
# End Source File
# Begin Source File

SOURCE=.\canonicalize.c
# End Source File
# Begin Source File

SOURCE=.\closeout.c
# End Source File
# Begin Source File

SOURCE=".\cycle-check.c"
# End Source File
# Begin Source File

SOURCE=.\dirname.c
# End Source File
# Begin Source File

SOURCE=".\dup-safer.c"
# End Source File
# Begin Source File

SOURCE=.\exitfail.c
# End Source File
# Begin Source File

SOURCE=".\fd-safer.c"
# End Source File
# Begin Source File

SOURCE=.\fncase.c
# End Source File
# Begin Source File

SOURCE=.\fnmatch.c
# End Source File
# Begin Source File

SOURCE=.\fseeko.c
# End Source File
# Begin Source File

SOURCE=.\ftello.c
# End Source File
# Begin Source File

SOURCE=.\gai_strerror.c
# End Source File
# Begin Source File

SOURCE=.\getaddrinfo.c
# End Source File
# Begin Source File

SOURCE=.\getdate.c
# End Source File
# Begin Source File

SOURCE=.\getdelim.c
# End Source File
# Begin Source File

SOURCE=.\getline.c
# End Source File
# Begin Source File

SOURCE=.\getlogin_r.c
# End Source File
# Begin Source File

SOURCE=.\getndelim2.c
# End Source File
# Begin Source File

SOURCE=.\getopt.c
# End Source File
# Begin Source File

SOURCE=.\getopt1.c
# End Source File
# Begin Source File

SOURCE=.\gettime.c
# End Source File
# Begin Source File

SOURCE=.\glob.c
# End Source File
# Begin Source File

SOURCE=.\lstat.c
# End Source File
# Begin Source File

SOURCE=.\mbchar.c
# End Source File
# Begin Source File

SOURCE=.\md5.c
# End Source File
# Begin Source File

SOURCE=.\mempcpy.c
# End Source File
# Begin Source File

SOURCE=.\mkstemp.c
# End Source File
# Begin Source File

SOURCE=.\pagealign_alloc.c
# End Source File
# Begin Source File

SOURCE=".\printf-args.c"
# End Source File
# Begin Source File

SOURCE=".\printf-parse.c"
# End Source File
# Begin Source File

SOURCE=.\quotearg.c
# End Source File
# Begin Source File

SOURCE=.\readlink.c
# End Source File
# Begin Source File

SOURCE=.\realloc.c
# End Source File
# Begin Source File

SOURCE=.\regex.c
# End Source File
# Begin Source File

SOURCE=.\rpmatch.c
# End Source File
# Begin Source File

SOURCE=".\save-cwd.c"
# End Source File
# Begin Source File

SOURCE=.\setenv.c
# End Source File
# Begin Source File

SOURCE=.\sighandle.c
# End Source File
# Begin Source File

SOURCE=.\strcasecmp.c
# End Source File
# Begin Source File

SOURCE=.\strftime.c
# End Source File
# Begin Source File

SOURCE=.\stripslash.c
# End Source File
# Begin Source File

SOURCE=.\strnlen1.c
# End Source File
# Begin Source File

SOURCE=.\tempname.c
# End Source File
# Begin Source File

SOURCE=.\time_r.c
# End Source File
# Begin Source File

SOURCE=.\unsetenv.c
# End Source File
# Begin Source File

SOURCE=.\vasnprintf.c
# End Source File
# Begin Source File

SOURCE=.\vasprintf.c
# End Source File
# Begin Source File

SOURCE=".\xalloc-die.c"
# End Source File
# Begin Source File

SOURCE=.\xgetcwd.c
# End Source File
# Begin Source File

SOURCE=.\xgethostname.c
# End Source File
# Begin Source File

SOURCE=.\xmalloc.c
# End Source File
# Begin Source File

SOURCE=.\xreadlink.c
# End Source File
# Begin Source File

SOURCE=.\yesno.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\__fpending.h
# End Source File
# Begin Source File

SOURCE=.\alloca.h
# End Source File
# Begin Source File

SOURCE=.\alloca_.h

!IF  "$(CFG)" == "libcvs - Win32 Release"

# Begin Custom Build
InputPath=.\alloca_.h

".\alloca.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\alloca_.h .\alloca.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libcvs - Win32 Debug"

# Begin Custom Build
InputPath=.\alloca_.h

".\alloca.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\alloca_.h .\alloca.h

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\allocsa.h
# End Source File
# Begin Source File

SOURCE=".\canon-host.h"
# End Source File
# Begin Source File

SOURCE=.\canonicalize.h
# End Source File
# Begin Source File

SOURCE=".\chdir-long.h"
# End Source File
# Begin Source File

SOURCE=.\closeout.h
# End Source File
# Begin Source File

SOURCE="..\windows-NT\config.h"
# End Source File
# Begin Source File

SOURCE=".\cycle-check.h"
# End Source File
# Begin Source File

SOURCE=".\dev-ino.h"
# End Source File
# Begin Source File

SOURCE=.\dirname.h
# End Source File
# Begin Source File

SOURCE=.\error.h
# End Source File
# Begin Source File

SOURCE=.\exit.h
# End Source File
# Begin Source File

SOURCE=.\exitfail.h
# End Source File
# Begin Source File

SOURCE=.\filenamecat.h
# End Source File
# Begin Source File

SOURCE=.\fnmatch.h
# End Source File
# Begin Source File

SOURCE=.\fnmatch_.h

!IF  "$(CFG)" == "libcvs - Win32 Release"

# Begin Custom Build
InputPath=.\fnmatch_.h

".\fnmatch.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\fnmatch_.h .\fnmatch.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libcvs - Win32 Debug"

# Begin Custom Build
InputPath=.\fnmatch_.h

".\fnmatch.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\fnmatch_.h .\fnmatch.h

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\getaddrinfo.h
# End Source File
# Begin Source File

SOURCE=.\getcwd.h
# End Source File
# Begin Source File

SOURCE=.\getdate.h
# End Source File
# Begin Source File

SOURCE=.\getdelim.h
# End Source File
# Begin Source File

SOURCE=.\getline.h
# End Source File
# Begin Source File

SOURCE=.\getlogin_r.h
# End Source File
# Begin Source File

SOURCE=.\getndelim2.h
# End Source File
# Begin Source File

SOURCE=.\getopt.h
# End Source File
# Begin Source File

SOURCE=.\getopt_.h

!IF  "$(CFG)" == "libcvs - Win32 Release"

# Begin Custom Build
InputPath=.\getopt_.h

".\getopt.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\getopt_.h .\getopt.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libcvs - Win32 Debug"

# Begin Custom Build
InputPath=.\getopt_.h

".\getopt.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\getopt_.h .\getopt.h

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\getopt_int.h
# End Source File
# Begin Source File

SOURCE=.\getpagesize.h
# End Source File
# Begin Source File

SOURCE=.\gettext.h
# End Source File
# Begin Source File

SOURCE=".\glob-libc.h"
# End Source File
# Begin Source File

SOURCE=.\glob.h
# End Source File
# Begin Source File

SOURCE=.\glob_.h

!IF  "$(CFG)" == "libcvs - Win32 Release"

# Begin Custom Build
InputPath=.\glob_.h

".\glob.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\glob_.h .\glob.h

# End Custom Build

!ELSEIF  "$(CFG)" == "libcvs - Win32 Debug"

# Begin Custom Build
InputPath=.\glob_.h

".\glob.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	copy .\glob_.h .\glob.h

# End Custom Build

!ENDIF 

# End Source File
# Begin Source File

SOURCE=.\mbchar.h
# End Source File
# Begin Source File

SOURCE=.\mbuiter.h
# End Source File
# Begin Source File

SOURCE=.\md5.h
# End Source File
# Begin Source File

SOURCE=.\mempcpy.h
# End Source File
# Begin Source File

SOURCE="..\windows-NT\ndir.h"
# End Source File
# Begin Source File

SOURCE="..\windows-NT\netdb.h"
# End Source File
# Begin Source File

SOURCE=.\pagealign_alloc.h
# End Source File
# Begin Source File

SOURCE=".\path-concat.h"
# End Source File
# Begin Source File

SOURCE=.\pathmax.h
# End Source File
# Begin Source File

SOURCE=".\printf-args.h"
# End Source File
# Begin Source File

SOURCE=".\printf-parse.h"
# End Source File
# Begin Source File

SOURCE="..\windows-NT\pwd.h"
# End Source File
# Begin Source File

SOURCE=.\quotearg.h
# End Source File
# Begin Source File

SOURCE=.\regex.h
# End Source File
# Begin Source File

SOURCE=.\regex_internal.h
# End Source File
# Begin Source File

SOURCE=".\save-cwd.h"
# End Source File
# Begin Source File

SOURCE=.\setenv.h
# End Source File
# Begin Source File

SOURCE="..\windows-NT\sys\socket.h"
# End Source File
# Begin Source File

SOURCE=".\stat-macros.h"
# End Source File
# Begin Source File

SOURCE="..\windows-NT\stdbool.h"
# End Source File
# Begin Source File

SOURCE="..\windows-NT\stdint.h"
# End Source File
# Begin Source File

SOURCE=.\strcase.h
# End Source File
# Begin Source File

SOURCE=.\strdup.h
# End Source File
# Begin Source File

SOURCE=.\strnlen1.h
# End Source File
# Begin Source File

SOURCE=.\system.h
# End Source File
# Begin Source File

SOURCE=.\time_r.h
# End Source File
# Begin Source File

SOURCE=.\timespec.h
# End Source File
# Begin Source File

SOURCE=".\unistd-safer.h"
# End Source File
# Begin Source File

SOURCE="..\windows-NT\unistd.h"
# End Source File
# Begin Source File

SOURCE=".\unlocked-io.h"
# End Source File
# Begin Source File

SOURCE=.\vasnprintf.h
# End Source File
# Begin Source File

SOURCE=.\vasprintf.h
# End Source File
# Begin Source File

SOURCE="..\windows-NT\woe32.h"
# End Source File
# Begin Source File

SOURCE=.\xalloc.h
# End Source File
# Begin Source File

SOURCE=.\xgetcwd.h
# End Source File
# Begin Source File

SOURCE=.\xgethostname.h
# End Source File
# Begin Source File

SOURCE=.\xreadlink.h
# End Source File
# Begin Source File

SOURCE=.\xsize.h
# End Source File
# Begin Source File

SOURCE=.\xtime.h
# End Source File
# Begin Source File

SOURCE=.\yesno.h
# End Source File
# End Group
# End Target
# End Project

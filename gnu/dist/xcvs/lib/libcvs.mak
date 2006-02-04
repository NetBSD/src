# Microsoft Developer Studio Generated NMAKE File, Based on libcvs.dsp
!IF "$(CFG)" == ""
CFG=libcvs - Win32 Debug
!MESSAGE No configuration specified. Defaulting to libcvs - Win32 Debug.
!ENDIF 

!IF "$(CFG)" != "libcvs - Win32 Release" && "$(CFG)" != "libcvs - Win32 Debug"
!MESSAGE Invalid configuration "$(CFG)" specified.
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
!ERROR An invalid configuration is specified.
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libcvs - Win32 Release"

OUTDIR=.\WinRel
INTDIR=.\WinRel
# Begin Custom Macros
OutDir=.\WinRel
# End Custom Macros

ALL : ".\fnmatch.h" "$(OUTDIR)\libcvs.lib"


CLEAN :
	-@erase "$(INTDIR)\argmatch.obj"
	-@erase "$(INTDIR)\fncase.obj"
	-@erase "$(INTDIR)\fnmatch.obj"
	-@erase "$(INTDIR)\getdate.obj"
	-@erase "$(INTDIR)\getline.obj"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getopt1.obj"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\regex.obj"
	-@erase "$(INTDIR)\savecwd.obj"
	-@erase "$(INTDIR)\sighandle.obj"
	-@erase "$(INTDIR)\stripslash.obj"
	-@erase "$(INTDIR)\valloc.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\xgetwd.obj"
	-@erase "$(INTDIR)\yesno.obj"
	-@erase "$(OUTDIR)\libcvs.lib"
	-@erase ".\fnmatch.h"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "..\windows-NT" /I "." /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /Fp"$(INTDIR)\libcvs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libcvs.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libcvs.lib" 
LIB32_OBJS= \
	"$(INTDIR)\argmatch.obj" \
	"$(INTDIR)\fncase.obj" \
	"$(INTDIR)\fnmatch.obj" \
	"$(INTDIR)\getdate.obj" \
	"$(INTDIR)\getline.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\getopt1.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\regex.obj" \
	"$(INTDIR)\savecwd.obj" \
	"$(INTDIR)\sighandle.obj" \
	"$(INTDIR)\stripslash.obj" \
	"$(INTDIR)\valloc.obj" \
	"$(INTDIR)\xgetwd.obj" \
	"$(INTDIR)\yesno.obj"

"$(OUTDIR)\libcvs.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ELSEIF  "$(CFG)" == "libcvs - Win32 Debug"

OUTDIR=.\WinDebug
INTDIR=.\WinDebug
# Begin Custom Macros
OutDir=.\WinDebug
# End Custom Macros

ALL : "$(OUTDIR)\libcvs.lib"


CLEAN :
	-@erase "$(INTDIR)\argmatch.obj"
	-@erase "$(INTDIR)\fncase.obj"
	-@erase "$(INTDIR)\fnmatch.obj"
	-@erase "$(INTDIR)\getdate.obj"
	-@erase "$(INTDIR)\getline.obj"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getopt1.obj"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\regex.obj"
	-@erase "$(INTDIR)\savecwd.obj"
	-@erase "$(INTDIR)\sighandle.obj"
	-@erase "$(INTDIR)\stripslash.obj"
	-@erase "$(INTDIR)\valloc.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\xgetwd.obj"
	-@erase "$(INTDIR)\yesno.obj"
	-@erase "$(OUTDIR)\libcvs.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "..\windows-NT" /I "." /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /Fp"$(INTDIR)\libcvs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libcvs.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libcvs.lib" 
LIB32_OBJS= \
	"$(INTDIR)\argmatch.obj" \
	"$(INTDIR)\fncase.obj" \
	"$(INTDIR)\fnmatch.obj" \
	"$(INTDIR)\getdate.obj" \
	"$(INTDIR)\getline.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\getopt1.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\regex.obj" \
	"$(INTDIR)\savecwd.obj" \
	"$(INTDIR)\sighandle.obj" \
	"$(INTDIR)\stripslash.obj" \
	"$(INTDIR)\valloc.obj" \
	"$(INTDIR)\xgetwd.obj" \
	"$(INTDIR)\yesno.obj"

"$(OUTDIR)\libcvs.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 

.c{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.obj::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.c{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cpp{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<

.cxx{$(INTDIR)}.sbr::
   $(CPP) @<<
   $(CPP_PROJ) $< 
<<


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("libcvs.dep")
!INCLUDE "libcvs.dep"
!ELSE 
!MESSAGE Warning: cannot find "libcvs.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "libcvs - Win32 Release" || "$(CFG)" == "libcvs - Win32 Debug"
SOURCE=.\argmatch.c

"$(INTDIR)\argmatch.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\fncase.c

"$(INTDIR)\fncase.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\fnmatch.c

"$(INTDIR)\fnmatch.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getdate.c

"$(INTDIR)\getdate.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getline.c

"$(INTDIR)\getline.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getopt.c

"$(INTDIR)\getopt.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getopt1.c

"$(INTDIR)\getopt1.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\md5.c

"$(INTDIR)\md5.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\regex.c

"$(INTDIR)\regex.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\savecwd.c

"$(INTDIR)\savecwd.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\sighandle.c

"$(INTDIR)\sighandle.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\stripslash.c

"$(INTDIR)\stripslash.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\valloc.c

"$(INTDIR)\valloc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\xgetwd.c

"$(INTDIR)\xgetwd.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\yesno.c

"$(INTDIR)\yesno.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\fnmatch.h.in

!IF  "$(CFG)" == "libcvs - Win32 Release"

InputPath=.\fnmatch.h.in

".\fnmatch.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	copy .\fnmatch.h.in .\fnmatch.h
<< 
	

!ELSEIF  "$(CFG)" == "libcvs - Win32 Debug"

InputPath=.\fnmatch.h.in

".\fnmatch.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	copy .\fnmatch.h.in .\fnmatch.h
<< 
	

!ENDIF 


!ENDIF 


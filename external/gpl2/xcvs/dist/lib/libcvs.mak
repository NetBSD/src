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

!IF  "$(CFG)" == "libcvs - Win32 Release"

OUTDIR=.\WinRel
INTDIR=.\WinRel
# Begin Custom Macros
OutDir=.\WinRel
# End Custom Macros

ALL : ".\glob.h" ".\getopt.h" ".\fnmatch.h" ".\alloca.h" "$(OUTDIR)\libcvs.lib"


CLEAN :
	-@erase "$(INTDIR)\__fpending.obj"
	-@erase "$(INTDIR)\asnprintf.obj"
	-@erase "$(INTDIR)\basename.obj"
	-@erase "$(INTDIR)\canon-host.obj"
	-@erase "$(INTDIR)\canonicalize.obj"
	-@erase "$(INTDIR)\closeout.obj"
	-@erase "$(INTDIR)\cycle-check.obj"
	-@erase "$(INTDIR)\dirname.obj"
	-@erase "$(INTDIR)\dup-safer.obj"
	-@erase "$(INTDIR)\exitfail.obj"
	-@erase "$(INTDIR)\fd-safer.obj"
	-@erase "$(INTDIR)\fncase.obj"
	-@erase "$(INTDIR)\fnmatch.obj"
	-@erase "$(INTDIR)\fseeko.obj"
	-@erase "$(INTDIR)\ftello.obj"
	-@erase "$(INTDIR)\gai_strerror.obj"
	-@erase "$(INTDIR)\getaddrinfo.obj"
	-@erase "$(INTDIR)\getdate.obj"
	-@erase "$(INTDIR)\getdelim.obj"
	-@erase "$(INTDIR)\getline.obj"
	-@erase "$(INTDIR)\getlogin_r.obj"
	-@erase "$(INTDIR)\getndelim2.obj"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getopt1.obj"
	-@erase "$(INTDIR)\gettime.obj"
	-@erase "$(INTDIR)\glob.obj"
	-@erase "$(INTDIR)\lstat.obj"
	-@erase "$(INTDIR)\mbchar.obj"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\mempcpy.obj"
	-@erase "$(INTDIR)\mkstemp.obj"
	-@erase "$(INTDIR)\pagealign_alloc.obj"
	-@erase "$(INTDIR)\printf-args.obj"
	-@erase "$(INTDIR)\printf-parse.obj"
	-@erase "$(INTDIR)\quotearg.obj"
	-@erase "$(INTDIR)\readlink.obj"
	-@erase "$(INTDIR)\realloc.obj"
	-@erase "$(INTDIR)\regex.obj"
	-@erase "$(INTDIR)\rpmatch.obj"
	-@erase "$(INTDIR)\save-cwd.obj"
	-@erase "$(INTDIR)\setenv.obj"
	-@erase "$(INTDIR)\sighandle.obj"
	-@erase "$(INTDIR)\strcasecmp.obj"
	-@erase "$(INTDIR)\strftime.obj"
	-@erase "$(INTDIR)\stripslash.obj"
	-@erase "$(INTDIR)\strnlen1.obj"
	-@erase "$(INTDIR)\tempname.obj"
	-@erase "$(INTDIR)\time_r.obj"
	-@erase "$(INTDIR)\unsetenv.obj"
	-@erase "$(INTDIR)\vasnprintf.obj"
	-@erase "$(INTDIR)\vasprintf.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\xalloc-die.obj"
	-@erase "$(INTDIR)\xgetcwd.obj"
	-@erase "$(INTDIR)\xgethostname.obj"
	-@erase "$(INTDIR)\xmalloc.obj"
	-@erase "$(INTDIR)\xreadlink.obj"
	-@erase "$(INTDIR)\yesno.obj"
	-@erase "$(OUTDIR)\libcvs.lib"
	-@erase ".\alloca.h"
	-@erase ".\fnmatch.h"
	-@erase ".\getopt.h"
	-@erase ".\glob.h"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /ML /W3 /GX /O2 /I "..\windows-NT" /I "." /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /Fp"$(INTDIR)\libcvs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /c 

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libcvs.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libcvs.lib" 
LIB32_OBJS= \
	"$(INTDIR)\__fpending.obj" \
	"$(INTDIR)\asnprintf.obj" \
	"$(INTDIR)\basename.obj" \
	"$(INTDIR)\canon-host.obj" \
	"$(INTDIR)\canonicalize.obj" \
	"$(INTDIR)\closeout.obj" \
	"$(INTDIR)\cycle-check.obj" \
	"$(INTDIR)\dirname.obj" \
	"$(INTDIR)\dup-safer.obj" \
	"$(INTDIR)\exitfail.obj" \
	"$(INTDIR)\fd-safer.obj" \
	"$(INTDIR)\fncase.obj" \
	"$(INTDIR)\fnmatch.obj" \
	"$(INTDIR)\fseeko.obj" \
	"$(INTDIR)\ftello.obj" \
	"$(INTDIR)\gai_strerror.obj" \
	"$(INTDIR)\getaddrinfo.obj" \
	"$(INTDIR)\getdate.obj" \
	"$(INTDIR)\getdelim.obj" \
	"$(INTDIR)\getline.obj" \
	"$(INTDIR)\getlogin_r.obj" \
	"$(INTDIR)\getndelim2.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\getopt1.obj" \
	"$(INTDIR)\gettime.obj" \
	"$(INTDIR)\glob.obj" \
	"$(INTDIR)\lstat.obj" \
	"$(INTDIR)\mbchar.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\mempcpy.obj" \
	"$(INTDIR)\mkstemp.obj" \
	"$(INTDIR)\pagealign_alloc.obj" \
	"$(INTDIR)\printf-args.obj" \
	"$(INTDIR)\printf-parse.obj" \
	"$(INTDIR)\quotearg.obj" \
	"$(INTDIR)\readlink.obj" \
	"$(INTDIR)\realloc.obj" \
	"$(INTDIR)\regex.obj" \
	"$(INTDIR)\rpmatch.obj" \
	"$(INTDIR)\save-cwd.obj" \
	"$(INTDIR)\setenv.obj" \
	"$(INTDIR)\sighandle.obj" \
	"$(INTDIR)\strcasecmp.obj" \
	"$(INTDIR)\strftime.obj" \
	"$(INTDIR)\stripslash.obj" \
	"$(INTDIR)\strnlen1.obj" \
	"$(INTDIR)\tempname.obj" \
	"$(INTDIR)\time_r.obj" \
	"$(INTDIR)\unsetenv.obj" \
	"$(INTDIR)\vasnprintf.obj" \
	"$(INTDIR)\vasprintf.obj" \
	"$(INTDIR)\xalloc-die.obj" \
	"$(INTDIR)\xgetcwd.obj" \
	"$(INTDIR)\xgethostname.obj" \
	"$(INTDIR)\xmalloc.obj" \
	"$(INTDIR)\xreadlink.obj" \
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
	-@erase "$(INTDIR)\__fpending.obj"
	-@erase "$(INTDIR)\asnprintf.obj"
	-@erase "$(INTDIR)\basename.obj"
	-@erase "$(INTDIR)\canon-host.obj"
	-@erase "$(INTDIR)\canonicalize.obj"
	-@erase "$(INTDIR)\closeout.obj"
	-@erase "$(INTDIR)\cycle-check.obj"
	-@erase "$(INTDIR)\dirname.obj"
	-@erase "$(INTDIR)\dup-safer.obj"
	-@erase "$(INTDIR)\exitfail.obj"
	-@erase "$(INTDIR)\fd-safer.obj"
	-@erase "$(INTDIR)\fncase.obj"
	-@erase "$(INTDIR)\fnmatch.obj"
	-@erase "$(INTDIR)\fseeko.obj"
	-@erase "$(INTDIR)\ftello.obj"
	-@erase "$(INTDIR)\gai_strerror.obj"
	-@erase "$(INTDIR)\getaddrinfo.obj"
	-@erase "$(INTDIR)\getdate.obj"
	-@erase "$(INTDIR)\getdelim.obj"
	-@erase "$(INTDIR)\getline.obj"
	-@erase "$(INTDIR)\getlogin_r.obj"
	-@erase "$(INTDIR)\getndelim2.obj"
	-@erase "$(INTDIR)\getopt.obj"
	-@erase "$(INTDIR)\getopt1.obj"
	-@erase "$(INTDIR)\gettime.obj"
	-@erase "$(INTDIR)\glob.obj"
	-@erase "$(INTDIR)\lstat.obj"
	-@erase "$(INTDIR)\mbchar.obj"
	-@erase "$(INTDIR)\md5.obj"
	-@erase "$(INTDIR)\mempcpy.obj"
	-@erase "$(INTDIR)\mkstemp.obj"
	-@erase "$(INTDIR)\pagealign_alloc.obj"
	-@erase "$(INTDIR)\printf-args.obj"
	-@erase "$(INTDIR)\printf-parse.obj"
	-@erase "$(INTDIR)\quotearg.obj"
	-@erase "$(INTDIR)\readlink.obj"
	-@erase "$(INTDIR)\realloc.obj"
	-@erase "$(INTDIR)\regex.obj"
	-@erase "$(INTDIR)\rpmatch.obj"
	-@erase "$(INTDIR)\save-cwd.obj"
	-@erase "$(INTDIR)\setenv.obj"
	-@erase "$(INTDIR)\sighandle.obj"
	-@erase "$(INTDIR)\strcasecmp.obj"
	-@erase "$(INTDIR)\strftime.obj"
	-@erase "$(INTDIR)\stripslash.obj"
	-@erase "$(INTDIR)\strnlen1.obj"
	-@erase "$(INTDIR)\tempname.obj"
	-@erase "$(INTDIR)\time_r.obj"
	-@erase "$(INTDIR)\unsetenv.obj"
	-@erase "$(INTDIR)\vasnprintf.obj"
	-@erase "$(INTDIR)\vasprintf.obj"
	-@erase "$(INTDIR)\vc60.idb"
	-@erase "$(INTDIR)\vc60.pdb"
	-@erase "$(INTDIR)\xalloc-die.obj"
	-@erase "$(INTDIR)\xgetcwd.obj"
	-@erase "$(INTDIR)\xgethostname.obj"
	-@erase "$(INTDIR)\xmalloc.obj"
	-@erase "$(INTDIR)\xreadlink.obj"
	-@erase "$(INTDIR)\yesno.obj"
	-@erase "$(OUTDIR)\libcvs.lib"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

CPP=cl.exe
CPP_PROJ=/nologo /MLd /W3 /Gm /GX /ZI /Od /I "..\windows-NT" /I "." /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /D "HAVE_CONFIG_H" /Fp"$(INTDIR)\libcvs.pch" /YX /Fo"$(INTDIR)\\" /Fd"$(INTDIR)\\" /FD /GZ /c 

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

RSC=rc.exe
BSC32=bscmake.exe
BSC32_FLAGS=/nologo /o"$(OUTDIR)\libcvs.bsc" 
BSC32_SBRS= \
	
LIB32=link.exe -lib
LIB32_FLAGS=/nologo /out:"$(OUTDIR)\libcvs.lib" 
LIB32_OBJS= \
	"$(INTDIR)\__fpending.obj" \
	"$(INTDIR)\asnprintf.obj" \
	"$(INTDIR)\basename.obj" \
	"$(INTDIR)\canon-host.obj" \
	"$(INTDIR)\canonicalize.obj" \
	"$(INTDIR)\closeout.obj" \
	"$(INTDIR)\cycle-check.obj" \
	"$(INTDIR)\dirname.obj" \
	"$(INTDIR)\dup-safer.obj" \
	"$(INTDIR)\exitfail.obj" \
	"$(INTDIR)\fd-safer.obj" \
	"$(INTDIR)\fncase.obj" \
	"$(INTDIR)\fnmatch.obj" \
	"$(INTDIR)\fseeko.obj" \
	"$(INTDIR)\ftello.obj" \
	"$(INTDIR)\gai_strerror.obj" \
	"$(INTDIR)\getaddrinfo.obj" \
	"$(INTDIR)\getdate.obj" \
	"$(INTDIR)\getdelim.obj" \
	"$(INTDIR)\getline.obj" \
	"$(INTDIR)\getlogin_r.obj" \
	"$(INTDIR)\getndelim2.obj" \
	"$(INTDIR)\getopt.obj" \
	"$(INTDIR)\getopt1.obj" \
	"$(INTDIR)\gettime.obj" \
	"$(INTDIR)\glob.obj" \
	"$(INTDIR)\lstat.obj" \
	"$(INTDIR)\mbchar.obj" \
	"$(INTDIR)\md5.obj" \
	"$(INTDIR)\mempcpy.obj" \
	"$(INTDIR)\mkstemp.obj" \
	"$(INTDIR)\pagealign_alloc.obj" \
	"$(INTDIR)\printf-args.obj" \
	"$(INTDIR)\printf-parse.obj" \
	"$(INTDIR)\quotearg.obj" \
	"$(INTDIR)\readlink.obj" \
	"$(INTDIR)\realloc.obj" \
	"$(INTDIR)\regex.obj" \
	"$(INTDIR)\rpmatch.obj" \
	"$(INTDIR)\save-cwd.obj" \
	"$(INTDIR)\setenv.obj" \
	"$(INTDIR)\sighandle.obj" \
	"$(INTDIR)\strcasecmp.obj" \
	"$(INTDIR)\strftime.obj" \
	"$(INTDIR)\stripslash.obj" \
	"$(INTDIR)\strnlen1.obj" \
	"$(INTDIR)\tempname.obj" \
	"$(INTDIR)\time_r.obj" \
	"$(INTDIR)\unsetenv.obj" \
	"$(INTDIR)\vasnprintf.obj" \
	"$(INTDIR)\vasprintf.obj" \
	"$(INTDIR)\xalloc-die.obj" \
	"$(INTDIR)\xgetcwd.obj" \
	"$(INTDIR)\xgethostname.obj" \
	"$(INTDIR)\xmalloc.obj" \
	"$(INTDIR)\xreadlink.obj" \
	"$(INTDIR)\yesno.obj"

"$(OUTDIR)\libcvs.lib" : "$(OUTDIR)" $(DEF_FILE) $(LIB32_OBJS)
    $(LIB32) @<<
  $(LIB32_FLAGS) $(DEF_FLAGS) $(LIB32_OBJS)
<<

!ENDIF 


!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("libcvs.dep")
!INCLUDE "libcvs.dep"
!ELSE 
!MESSAGE Warning: cannot find "libcvs.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "libcvs - Win32 Release" || "$(CFG)" == "libcvs - Win32 Debug"
SOURCE=.\__fpending.c

"$(INTDIR)\__fpending.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\asnprintf.c

"$(INTDIR)\asnprintf.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\basename.c

"$(INTDIR)\basename.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\canon-host.c"

"$(INTDIR)\canon-host.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\canonicalize.c

"$(INTDIR)\canonicalize.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\closeout.c

"$(INTDIR)\closeout.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\cycle-check.c"

"$(INTDIR)\cycle-check.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\dirname.c

"$(INTDIR)\dirname.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\dup-safer.c"

"$(INTDIR)\dup-safer.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\exitfail.c

"$(INTDIR)\exitfail.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\fd-safer.c"

"$(INTDIR)\fd-safer.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\fncase.c

"$(INTDIR)\fncase.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\fnmatch.c

"$(INTDIR)\fnmatch.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\fseeko.c

"$(INTDIR)\fseeko.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\ftello.c

"$(INTDIR)\ftello.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\gai_strerror.c

"$(INTDIR)\gai_strerror.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getaddrinfo.c

"$(INTDIR)\getaddrinfo.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getdate.c

"$(INTDIR)\getdate.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getdelim.c

"$(INTDIR)\getdelim.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getline.c

"$(INTDIR)\getline.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getlogin_r.c

"$(INTDIR)\getlogin_r.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getndelim2.c

"$(INTDIR)\getndelim2.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getopt.c

"$(INTDIR)\getopt.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\getopt1.c

"$(INTDIR)\getopt1.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\gettime.c

"$(INTDIR)\gettime.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\glob.c

"$(INTDIR)\glob.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\lstat.c

"$(INTDIR)\lstat.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\mbchar.c

"$(INTDIR)\mbchar.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\md5.c

"$(INTDIR)\md5.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\mempcpy.c

"$(INTDIR)\mempcpy.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\mkstemp.c

"$(INTDIR)\mkstemp.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\pagealign_alloc.c

"$(INTDIR)\pagealign_alloc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\printf-args.c"

"$(INTDIR)\printf-args.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\printf-parse.c"

"$(INTDIR)\printf-parse.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\quotearg.c

"$(INTDIR)\quotearg.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\readlink.c

"$(INTDIR)\readlink.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\realloc.c

"$(INTDIR)\realloc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\regex.c

"$(INTDIR)\regex.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\rpmatch.c

"$(INTDIR)\rpmatch.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\save-cwd.c"

"$(INTDIR)\save-cwd.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\setenv.c

"$(INTDIR)\setenv.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\sighandle.c

"$(INTDIR)\sighandle.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\strcasecmp.c

"$(INTDIR)\strcasecmp.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\strftime.c

"$(INTDIR)\strftime.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\stripslash.c

"$(INTDIR)\stripslash.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\strnlen1.c

"$(INTDIR)\strnlen1.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\tempname.c

"$(INTDIR)\tempname.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\time_r.c

"$(INTDIR)\time_r.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\unsetenv.c

"$(INTDIR)\unsetenv.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\vasnprintf.c

"$(INTDIR)\vasnprintf.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\vasprintf.c

"$(INTDIR)\vasprintf.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=".\xalloc-die.c"

"$(INTDIR)\xalloc-die.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\xgetcwd.c

"$(INTDIR)\xgetcwd.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\xgethostname.c

"$(INTDIR)\xgethostname.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\xmalloc.c

"$(INTDIR)\xmalloc.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\xreadlink.c

"$(INTDIR)\xreadlink.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\yesno.c

"$(INTDIR)\yesno.obj" : $(SOURCE) "$(INTDIR)"


SOURCE=.\alloca_.h

!IF  "$(CFG)" == "libcvs - Win32 Release"

InputPath=.\alloca_.h

".\alloca.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	copy .\alloca_.h .\alloca.h
<< 
	

!ELSEIF  "$(CFG)" == "libcvs - Win32 Debug"

InputPath=.\alloca_.h

".\alloca.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	copy .\alloca_.h .\alloca.h
<< 
	

!ENDIF 

SOURCE=.\fnmatch_.h

!IF  "$(CFG)" == "libcvs - Win32 Release"

InputPath=.\fnmatch_.h

".\fnmatch.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	copy .\fnmatch_.h .\fnmatch.h
<< 
	

!ELSEIF  "$(CFG)" == "libcvs - Win32 Debug"

InputPath=.\fnmatch_.h

".\fnmatch.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	copy .\fnmatch_.h .\fnmatch.h
<< 
	

!ENDIF 

SOURCE=.\getopt_.h

!IF  "$(CFG)" == "libcvs - Win32 Release"

InputPath=.\getopt_.h

".\getopt.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	copy .\getopt_.h .\getopt.h
<< 
	

!ELSEIF  "$(CFG)" == "libcvs - Win32 Debug"

InputPath=.\getopt_.h

".\getopt.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	copy .\getopt_.h .\getopt.h
<< 
	

!ENDIF 

SOURCE=.\glob_.h

!IF  "$(CFG)" == "libcvs - Win32 Release"

InputPath=.\glob_.h

".\glob.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	copy .\glob_.h .\glob.h
<< 
	

!ELSEIF  "$(CFG)" == "libcvs - Win32 Debug"

InputPath=.\glob_.h

".\glob.h" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	copy .\glob_.h .\glob.h
<< 
	

!ENDIF 


!ENDIF 


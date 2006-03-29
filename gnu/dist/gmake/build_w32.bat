set make=gnumake
+if not exist config.h copy config.h.W32 config.h
cd w32\subproc
echo "Creating the subproc library"
%ComSpec% /c build.bat
cd ..\..
del link.dbg link.rel
del config.h
copy config.h.W32 config.h
echo off
echo "Creating GNU make for Windows 95/NT"
echo on
if not exist .\WinDebug\nul mkdir .\WinDebug
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D TIVOLI /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c variable.c
echo WinDebug\variable.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c rule.c
echo WinDebug\rule.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c remote-stub.c
echo WinDebug\remote-stub.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c commands.c
echo WinDebug\commands.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c file.c
echo WinDebug\file.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c getloadavg.c
echo WinDebug\getloadavg.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c default.c
echo WinDebug\default.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c signame.c
echo WinDebug\signame.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c expand.c
echo WinDebug\expand.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c dir.c
echo WinDebug\dir.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c main.c
echo WinDebug\main.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c getopt1.c
echo WinDebug\getopt1.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c job.c
echo WinDebug\job.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c read.c
echo WinDebug\read.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c version.c
echo WinDebug\version.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c getopt.c
echo WinDebug\getopt.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c arscan.c
echo WinDebug\arscan.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c remake.c
echo WinDebug\remake.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c misc.c
echo WinDebug\misc.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c ar.c
echo WinDebug\ar.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c function.c
echo WinDebug\function.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c vpath.c
echo WinDebug\vpath.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c implicit.c
echo WinDebug\implicit.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c  .\w32\compat\dirent.c
echo WinDebug\dirent.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c  .\glob\glob.c
echo WinDebug\glob.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c  .\glob\fnmatch.c
echo WinDebug\fnmatch.obj >>link.dbg
cl.exe /nologo /MT /W3 /GX /Zi /YX /Od /I . /I glob /I w32/include /D _DEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinDebug/ /Fp.\WinDebug/%make%.pch /Fo.\WinDebug/ /Fd.\WinDebug/%make%.pdb /c  .\w32\pathstuff.c
echo WinDebug\pathstuff.obj >>link.dbg
echo off
echo "Linking WinDebug/%make%.exe"
rem link.exe kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib w32\subproc\windebug\subproc.lib /NOLOGO /SUBSYSTEM:console /INCREMENTAL:yes /PDB:.\WinDebug/%make%.pdb /DEBUG /MACHINE:I386 /OUT:.\WinDebug/%make%.exe .\WinDebug/variable.obj  .\WinDebug/rule.obj  .\WinDebug/remote-stub.obj  .\WinDebug/commands.obj  .\WinDebug/file.obj  .\WinDebug/getloadavg.obj  .\WinDebug/default.obj  .\WinDebug/signame.obj  .\WinDebug/expand.obj  .\WinDebug/dir.obj  .\WinDebug/main.obj  .\WinDebug/getopt1.obj  .\WinDebug/job.obj  .\WinDebug/read.obj  .\WinDebug/version.obj  .\WinDebug/getopt.obj  .\WinDebug/arscan.obj  .\WinDebug/remake.obj  .\WinDebug/misc.obj  .\WinDebug/ar.obj  .\WinDebug/function.obj  .\WinDebug/vpath.obj  .\WinDebug/implicit.obj  .\WinDebug/dirent.obj  .\WinDebug/glob.obj  .\WinDebug/fnmatch.obj  .\WinDebug/pathstuff.obj
echo kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib w32\subproc\windebug\subproc.lib >>link.dbg
link.exe /NOLOGO /SUBSYSTEM:console /INCREMENTAL:yes /PDB:.\WinDebug/%make%.pdb /DEBUG /MACHINE:I386 /OUT:.\WinDebug/%make%.exe @link.dbg
if not exist .\WinDebug/%make%.exe echo "WinDebug build failed"
if exist .\WinDebug/%make%.exe echo "WinDebug build succeeded!"
if not exist .\WinRel\nul mkdir .\WinRel
echo on
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /D TIVOLI /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c variable.c
echo WinRel\variable.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c rule.c
echo WinRel\rule.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c remote-stub.c
echo WinRel\remote-stub.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c commands.c
echo WinRel\commands.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c file.c
echo WinRel\file.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c getloadavg.c
echo WinRel\getloadavg.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c default.c
echo WinRel\default.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c signame.c
echo WinRel\signame.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c expand.c
echo WinRel\expand.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c dir.c
echo WinRel\dir.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c main.c
echo WinRel\main.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c getopt1.c
echo WinRel\getopt1.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c job.c
echo WinRel\job.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c read.c
echo WinRel\read.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c version.c
echo WinRel\version.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c getopt.c
echo WinRel\getopt.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c arscan.c
echo WinRel\arscan.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c remake.c
echo WinRel\remake.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c misc.c
echo WinRel\misc.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c ar.c
echo WinRel\ar.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c function.c
echo WinRel\function.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c vpath.c
echo WinRel\vpath.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c implicit.c
echo WinRel\implicit.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c  .\w32\compat\dirent.c
echo WinRel\dirent.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c  .\glob\glob.c
echo WinRel\glob.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c  .\glob\fnmatch.c
echo WinRel\fnmatch.obj >>link.rel
cl.exe /nologo /MT /W3 /GX /YX /O2 /I . /I glob /I w32/include /D NDEBUG /D WINDOWS32 /D WIN32 /D _CONSOLE /D HAVE_CONFIG_H /FR.\WinRel/ /Fp.\WinRel/%make%.pch /Fo.\WinRel/ /c  .\w32\pathstuff.c
echo WinRel\pathstuff.obj >>link.rel
echo off
echo "Linking WinRel/%make%.exe"
rem link.exe kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib w32\subproc\winrel\subproc.lib /NOLOGO /SUBSYSTEM:console /INCREMENTAL:no /PDB:.\WinRel/%make%.pdb /MACHINE:I386 /OUT:.\WinRel/%make%.exe .\WinRel/variable.obj  .\WinRel/rule.obj  .\WinRel/remote-stub.obj  .\WinRel/commands.obj  .\WinRel/file.obj  .\WinRel/getloadavg.obj  .\WinRel/default.obj  .\WinRel/signame.obj  .\WinRel/expand.obj  .\WinRel/dir.obj  .\WinRel/main.obj  .\WinRel/getopt1.obj  .\WinRel/job.obj  .\WinRel/read.obj  .\WinRel/version.obj  .\WinRel/getopt.obj  .\WinRel/arscan.obj  .\WinRel/remake.obj  .\WinRel/misc.obj  .\WinRel/ar.obj  .\WinRel/function.obj  .\WinRel/vpath.obj  .\WinRel/implicit.obj  .\WinRel/dirent.obj  .\WinRel/glob.obj  .\WinRel/fnmatch.obj  .\WinRel/pathstuff.obj
echo kernel32.lib user32.lib gdi32.lib winspool.lib comdlg32.lib advapi32.lib shell32.lib ole32.lib oleaut32.lib uuid.lib odbc32.lib odbccp32.lib w32\subproc\winrel\subproc.lib >>link.rel
link.exe /NOLOGO /SUBSYSTEM:console /INCREMENTAL:no /PDB:.\WinRel/%make%.pdb /MACHINE:I386 /OUT:.\WinRel/%make%.exe @link.rel
if not exist .\WinRel/%make%.exe echo "WinRel build failed"
if exist .\WinRel/%make%.exe echo "WinRel build succeeded!"
echo on

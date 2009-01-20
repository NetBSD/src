# Microsoft Developer Studio Project File - Name="libops" - Package Owner=<4>
# Microsoft Developer Studio Generated Build File, Format Version 6.00
# ** DO NOT EDIT **

# TARGTYPE "Win32 (x86) Static Library" 0x0104

CFG=libops - Win32 Debug
!MESSAGE This is not a valid makefile. To build this project using NMAKE,
!MESSAGE use the Export Makefile command and run
!MESSAGE 
!MESSAGE NMAKE /f "libops.mak".
!MESSAGE 
!MESSAGE You can specify a configuration when running NMAKE
!MESSAGE by defining the macro CFG on the command line. For example:
!MESSAGE 
!MESSAGE NMAKE /f "libops.mak" CFG="libops - Win32 Debug"
!MESSAGE 
!MESSAGE Possible choices for configuration are:
!MESSAGE 
!MESSAGE "libops - Win32 Release" (based on "Win32 (x86) Static Library")
!MESSAGE "libops - Win32 Debug" (based on "Win32 (x86) Static Library")
!MESSAGE 

# Begin Project
# PROP AllowPerConfigDependencies 0
# PROP Scc_ProjName ""
# PROP Scc_LocalPath ""
CPP=cl.exe
RSC=rc.exe

!IF  "$(CFG)" == "libops - Win32 Release"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 0
# PROP BASE Output_Dir "Release"
# PROP BASE Intermediate_Dir "Release"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 0
# PROP Output_Dir "Release"
# PROP Intermediate_Dir "Release"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /GX /O2 /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /YX /FD /c
# ADD CPP /nologo /MD /W3 /GX /O2 /I ".\advanced" /I "..\include" /I "..\..\openssl\include" /I "..\..\zlib\include" /D "WIN32" /D "NDEBUG" /D "_MBCS" /D "_LIB" /FD /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x422 /d "NDEBUG"
# ADD RSC /l 0x422 /d "NDEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\win32\Release\libops.lib"

!ELSEIF  "$(CFG)" == "libops - Win32 Debug"

# PROP BASE Use_MFC 0
# PROP BASE Use_Debug_Libraries 1
# PROP BASE Output_Dir "Debug"
# PROP BASE Intermediate_Dir "Debug"
# PROP BASE Target_Dir ""
# PROP Use_MFC 0
# PROP Use_Debug_Libraries 1
# PROP Output_Dir "Debug"
# PROP Intermediate_Dir "Debug"
# PROP Target_Dir ""
# ADD BASE CPP /nologo /W3 /Gm /GX /ZI /Od /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /YX /FD /GZ  /c
# ADD CPP /nologo /MDd /W3 /Gm /GX /ZI /Od /I ".\advanced" /I "..\include" /I "..\..\openssl\include" /I "..\..\zlib\include" /D "WIN32" /D "_DEBUG" /D "_MBCS" /D "_LIB" /FD /GZ  /c
# SUBTRACT CPP /YX
# ADD BASE RSC /l 0x422 /d "_DEBUG"
# ADD RSC /l 0x422 /d "_DEBUG"
BSC32=bscmake.exe
# ADD BASE BSC32 /nologo
# ADD BSC32 /nologo
LIB32=link.exe -lib
# ADD BASE LIB32 /nologo
# ADD LIB32 /nologo /out:"..\lib\win32\Debug\libops.lib"

!ENDIF 

# Begin Target

# Name "libops - Win32 Release"
# Name "libops - Win32 Debug"
# Begin Group "Source Files"

# PROP Default_Filter "cpp;c;cxx;rc;def;r;odl;idl;hpj;bat"
# Begin Source File

SOURCE=.\advanced\adv_accumulate.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_armour.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_compress.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_create.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_crypto.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_errors.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_fingerprint.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_hash.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_keyring.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_lists.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_memory.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_openssl_crypto.c
# End Source File
# Begin Source File

SOURCE=".\advanced\adv_packet-parse.c"
# End Source File
# Begin Source File

SOURCE=".\advanced\adv_packet-show.c"
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_readerwriter.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_signature.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_symmetric.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_util.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_validate.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_writer_encrypt.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_writer_encrypt_se_ip.c
# End Source File
# Begin Source File

SOURCE=.\advanced\adv_writer_stream_encrypt_se_ip.c
# End Source File
# Begin Source File

SOURCE=.\advanced\random.c
# End Source File
# Begin Source File

SOURCE=.\standard\std_keyring.c
# End Source File
# Begin Source File

SOURCE=.\standard\std_print.c
# End Source File
# End Group
# Begin Group "Header Files"

# PROP Default_Filter "h;hpp;hxx;hm;inl"
# Begin Source File

SOURCE=.\advanced\keyring_local.h
# End Source File
# Begin Source File

SOURCE=.\advanced\parse_local.h
# End Source File
# End Group
# End Target
# End Project

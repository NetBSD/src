$ ! Set the LOGICAL SYS to SYS$LIBRARY if it isn't defined
$ ! so that #include <sys/...> will search SYS$LIBRARY for the file.
$ IF F$TRNLNM("SYS") .EQS. "" THEN DEFINE SYS SYS$LIBRARY
$ ccopt = "/include=sys$disk:[]"
$ if (f$search("sys$system:decc$compiler.exe").nes."")
$ then
$   if f$getsyi("HW_MODEL").ge.1024
$   then
$     ccopt = "/prefix=all" + ccopt
$   else
$     ccopt = "/decc/prefix=all" + ccopt
$   endif
$ endif
$ if f$extract(1,3,f$getsyi("Version")) .lts. "7.0"
$ then 
$   if f$search("x11vms:xvmsutils.olb").eqs.""
$   then
$     type sys$input
To build grep on OpenVMS versions prior to 7.x you need to install the 
xvmsutils to get the close-/open-/readdir functions. The library can be 
found at http://www.decus.de:8080/www/vms/sw/xvmsutils.htmlx

Exiting now
$     exit
$   endif
$   llib = ",x11vms:xvmsutils.olb/lib"
$ else
$   llib = ""
$ endif
$ copy [-.vms]config_vms.h config.h
$ cdefs ="HAVE_CONFIG_H"
$ WRITE SYS$OUTPUT "Compiling ALLOCA..."
$ CC'ccopt' ALLOCA.C   /DEFINE=('cdefs')
$ WRITE SYS$OUTPUT "Compiling DFA..."
$ CC'ccopt' DFA.C      /DEFINE=('cdefs')
$ WRITE SYS$OUTPUT "Compiling GETOPT..."
$ CC'ccopt' GETOPT.C   /DEFINE=('cdefs')
$ WRITE SYS$OUTPUT "Compiling GETOPT1..."
$ CC'ccopt' GETOPT1.C   /DEFINE=('cdefs')
$ WRITE SYS$OUTPUT "Compiling GREP..."
$ CC'ccopt' GREP.C     /DEFINE=('cdefs',initialize_main="vms_fab")
$ WRITE SYS$OUTPUT "Compiling KWSET..."
$ CC'ccopt' KWSET.C    /DEFINE=('cdefs')
$ WRITE SYS$OUTPUT "Compiling OBSTACK..."
$ CC'ccopt' OBSTACK.C  /DEFINE=('cdefs')
$ WRITE SYS$OUTPUT "Compiling REGEX..."
$ CC'ccopt' REGEX.C    /DEFINE=('cdefs')
$ WRITE SYS$OUTPUT "Compiling SAVEDIR..."
$ CC'ccopt' SAVEDIR.C   /DEFINE=('cdefs')
$ WRITE SYS$OUTPUT "Compiling SEARCH..."
$ CC'ccopt' SEARCH.C   /DEFINE=('cdefs')
$ WRITE SYS$OUTPUT "Compiling STPCPY..."
$ CC'ccopt' STPCPY.C   /DEFINE=('cdefs')
$ WRITE SYS$OUTPUT "Compiling VMS_FAB..."
$ CC'ccopt' VMS_FAB.C   /DEFINE=('cdefs')
$ WRITE SYS$OUTPUT "Compiling GREPMAT..."
$ CC'ccopt' GREPMAT.C   /DEFINE=('cdefs')
$ WRITE SYS$OUTPUT "Linking GREP..."
$ LINK GREP,ALLOCA,DFA,GETOPT,GETOPT1,KWSET,OBSTACK,REGEX,SEARCH,VMS_FAB,-
       SAVEDIR,STPCPY,GREPMAT'llib'
$ WRITE SYS$OUTPUT "Compiling EGREPMAT..."
$ CC'ccopt' EGREPMAT.C   /DEFINE=('cdefs')
$ WRITE SYS$OUTPUT "Linking EGREP..."
$ LINK/exe=egrep.exe grep,ALLOCA,DFA,GETOPT,GETOPT1,KWSET,OBSTACK,REGEX,SEARCH,-
       VMS_FAB,SAVEDIR,STPCPY,EGREPMAT'llib'
$ WRITE SYS$OUTPUT "Compiling FGREPMAT..."
$ CC'ccopt' FGREPMAT.C   /DEFINE=('cdefs')
$ WRITE SYS$OUTPUT "Linking FGREP..."
$ LINK/exe=fgrep.exe GREP,ALLOCA,DFA,GETOPT,GETOPT1,KWSET,OBSTACK,REGEX,SEARCH,-
       VMS_FAB,savedir,stpcpy,FGREPMAT'llib'
$ EXIT

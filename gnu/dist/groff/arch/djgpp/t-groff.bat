@echo off
Rem This script runs groff without requiring that it be installed.
Rem The current directory must be the build directory.

test -d ./src/roff/groff
if not errorlevel 1 goto dirOk
echo this batch file must be run with the build directory as the current directory
goto end
:dirOk
test -x ./src/roff/groff/groff
if not errorlevel 1 goto groffOk
echo this batch file must be run with the build directory as the current directory
goto end
:groffOk
Rem chdir to src, to avoid overflowing the DOS limits with a long PATH.
cd src
set GROFF_FONT_PATH=..;../font
set GROFF_TMAC_PATH=../tmac;./roff/troff
set PATH1=%PATH%
set PATH=roff\troff;preproc\pic;preproc\eqn;preproc\tbl;preproc\grn;preproc\refer;preproc\soelim;devices\grotty;%PATH1%
Rem
echo I will use this command to format a document and print it on the screen:
roff\groff\groff -V -e -s -t -p -R -m ttchar -me -Tascii ../doc/meintro.me
pause
Rem Make the following command pipe to Less if you have Less installed:
roff\groff\groff -e -s -t -p -R -m ttchar -me -Tascii ../doc/meintro.me
Rem
set PATH=roff\troff;preproc\pic;preproc\eqn;preproc\tbl;preproc\grn;preproc\refer;preproc\soelim;devices\grops;%PATH1%
echo I will use this command to format a document and print it on PS printer:
roff\groff\groff -V -e -s -t -p -R -me -Tps ../doc/meintro.me
pause
Rem Uncomment the following command if you have a PostScript printer:
REM roff\groff\groff -e -s -t -p -R -me -Tps ../doc/meintro.me >>prn
Rem
set PATH=roff\troff;preproc\pic;preproc\eqn;preproc\tbl;preproc\grn;preproc\refer;preproc\soelim;devices\grolj4;%PATH1%
echo I will use this command to format a document and print it on LJ4 printer:
roff\groff\groff -V -e -s -t -p -R -me -Tlj4 ../doc/meintro.me
pause
Rem Uncomment the following command if you have a LaserJet4 printer:
REM roff\groff\groff -e -s -t -p -R -me -Tlj4 ../doc/meintro.me >>prn
Rem
set PATH=roff\troff;preproc\pic;preproc\eqn;preproc\tbl;preproc\grn;preproc\refer;preproc\soelim;devices\grodvi;%PATH1%
echo I will use this command to format a document into a DVI format:
roff\groff\groff -V -e -s -t -p -R -me -Tdvi ../doc/meintro.me
pause
roff\groff\groff -e -s -t -p -R -me -Tdvi ../doc/meintro.me > meintro.dvi
set PATH=roff\troff;preproc\pic;preproc\eqn;preproc\tbl;preproc\grn;preproc\refer;preproc\soelim;devices\grohtml;%PATH1%
echo I will use this command to format a document into HTML format:
roff\groff\groff -V -e -s -t -p -R -me -Thtml ../doc/meintro.me
pause
roff\groff\groff -e -s -t -p -R -me -Thtml ../doc/meintro.me > meintro.html
set PATH=roff\troff;preproc\pic;preproc\eqn;preproc\tbl;preproc\grn;preproc\refer;preproc\soelim;devices\grolbp;%PATH1%
echo I will use this command to format a document and print it on an LBP printer:
roff\groff\groff -V -e -s -t -p -R -me -Tlbp ../doc/meintro.me
pause
Rem Uncomment the following if you have a Canon CAPSL LBP-4 or LBP-8 printer:
REM roff\groff\groff -e -s -t -p -R -me -Tlbp ../doc/meintro.me >>prn
Rem
set PATH=%PATH1%
set GROFF_FONT_PATH=
set GROFF_TMAC_PATH=
set PATH1=
cd ..
:exit

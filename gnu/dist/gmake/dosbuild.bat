@echo Building Make for MSDOS
@rem Echo ON so they will see what is going on.
@echo on
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g commands.c -o commands.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g job.c -o job.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g dir.c -o dir.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g file.c -o file.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g misc.c -o misc.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g main.c -o main.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -DINCLUDEDIR=\"c:/djgpp/include\" -O2 -g read.c -o read.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -DLIBDIR=\"c:/djgpp/lib\" -O2 -g remake.c -o remake.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g rule.c -o rule.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g implicit.c -o implicit.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g default.c -o default.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g variable.c -o variable.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g expand.c -o expand.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g function.c -o function.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g vpath.c -o vpath.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g version.c -o version.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g ar.c -o ar.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g arscan.c -o arscan.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g signame.c -o signame.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g remote-stub.c -o remote-stub.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g getopt.c -o getopt.o
gcc  -c -I. -I./glob -DHAVE_CONFIG_H -O2 -g getopt1.c -o getopt1.o
@cd glob
@if exist libglob.a del libglob.a
gcc -I. -c -DHAVE_CONFIG_H -I.. -O2 -g glob.c -o glob.o
gcc -I. -c -DHAVE_CONFIG_H -I.. -O2 -g fnmatch.c -o fnmatch.o
ar rv libglob.a glob.o fnmatch.o
@echo off
cd ..
echo commands.o > respf.$$$
for %%f in (job dir file misc main read remake rule implicit default variable) do echo %%f.o >> respf.$$$
for %%f in (expand function vpath version ar arscan signame remote-stub getopt getopt1) do echo %%f.o >> respf.$$$
echo glob/libglob.a >> respf.$$$
@echo Linking...
@echo on
gcc -o make.new @respf.$$$
@if exist make.exe echo Make.exe is now built!
@if not exist make.exe echo Make.exe build failed...
@if exist make.exe del respf.$$$

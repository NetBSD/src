#name: Bad barrier options (Thumb)
#skip: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd
#source: barrier-bad.s
#as: -mthumb
#error-output: barrier-bad.l

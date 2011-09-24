# name: Cannot use flag-variant of PSR on v7m and v6m.
# skip: *-*-*coff *-*-pe *-*-wince *-*-*aout* *-*-netbsd
# error-output: msr-reg-bad.l
# source: msr-reg.s
# as: -march=armv7-m

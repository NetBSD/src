#source: ehdr_start-strongref.s
#ld: -e _start -T ehdr_start-missing.t
#error: .*: undefined reference to `__ehdr_start'
#target: *-*-linux* *-*-gnu* *-*-nacl*

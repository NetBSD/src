#source: tlstoc.s
#as: -a64
#ld: -shared
#objdump: -sj.tdata
#target: powerpc64*-*-*

.*

Contents of section \.tdata:
.* (12345678|78563412) (9abcdef0|f0debc9a) (23456789|89674523) (abcdef01|01efcdab)  .*
.* (3456789a|9a785634) (bcdef012|12f0debc) (456789ab|ab896745) (cdef0123|2301efcd)  .*
.* (56789abc|bc9a7856) (def01234|3412f0de) (6789abcd|cdab8967) (ef012345|452301ef)  .*
.* (789abcde|debc9a78) (f0123456|563412f0)                    .*

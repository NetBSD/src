#source: tls.s
#source: tlslib.s
#as: -a64
#ld: 
#objdump: -sj.got
#target: powerpc64*-*-*

.*

Contents of section \.got:
 100101e0 (00000000|e0810110) (100181e0|00000000) (ffffffff|1880ffff) (ffff8018|ffffffff)  .*
 100101f0 (ffffffff|5880ffff) (ffff8058|ffffffff)                    .*

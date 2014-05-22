#objdump: -rs -j .text
#name: forward expression

.*: .*

RELOCATION RECORDS FOR .*
OFFSET +TYPE +VALUE 
0+ .*(\.data|i)(|\+0xf+e|\+0xf+c|\+0xf+8|-0x0*2|-0x0*4|-0x0*8)

Contents of section .*
 0+ (0+|feff|fffe|fcffffff|fffffffc|f8ffffff|f8ffffff ffffffff|ffffffff fffffff8) .*

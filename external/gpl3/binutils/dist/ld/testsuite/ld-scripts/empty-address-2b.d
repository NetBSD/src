#source: empty-address-2.s
#ld: -Ttext 0x0000000 -Tdata 0x2000000 -T empty-address-2b.t
#nm: -n
#notarget: frv-*-*linux* *-*-linux*aout *-*-linux*oldld
#...
0+0 T _start
#...
0+10 [ADT] __data_end
#pass

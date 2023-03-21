#source: merge3.s
#ld: -T merge.ld
#objdump: -s
#xfail: [is_generic] hppa64-*-* ip2k-*-*

.*:     file format .*elf.*

Contents of section \.text:
 1000 (20100000|00001020) (10100000|00001010) (18100000|00001018) .*
Contents of section \.rodata:
 1010 64656667 00000000 30313233 34353637  defg....01234567
 1020 61626364 65666700                    abcdefg.        
#pass

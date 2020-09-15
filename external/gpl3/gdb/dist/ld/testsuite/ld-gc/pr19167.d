#source: pr19167a.s
#source: pr19167b.s
#ld: --gc-sections -e _start
#objdump: -s -j _foo
#target: *-*-linux* *-*-gnu* arm*-*-uclinuxfdpiceabi
#xfail: frv-*-* metag-*-*

#...
Contents of section _foo:
 [0-9a-f]+ [0-9a-f]+ [0-9a-f]+ [0-9a-f]+ [0-9a-f]+[ \t]+This is a test.*
#pass

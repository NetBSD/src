#name: --gc-sections with __start_
#ld: --gc-sections -e _start
#nm: -n
#target: *-*-linux* *-*-gnu* arm*-*-uclinuxfdpiceabi
#xfail: frv-*-* metag-*-*

#...
[0-9a-f]+ D +__start__foo
#...

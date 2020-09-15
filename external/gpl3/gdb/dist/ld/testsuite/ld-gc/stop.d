#name: --gc-sections removing __stop_
#ld: --gc-sections -e _start
#nm: -n
#target: *-*-linux* *-*-gnu* arm*-*-uclinuxfdpiceabi

#failif
#...
[0-9a-f]+ D +__stop__foo
#...

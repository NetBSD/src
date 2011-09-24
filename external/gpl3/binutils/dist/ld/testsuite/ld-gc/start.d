#name: --gc-sections with __start_
#ld: --gc-sections -e _start
#nm: -n
#target: *-*-linux* *-*-gnu*
#notarget: *-*-*aout *-*-*oldld frv-*-linux*

#...
[0-9a-f]+ A +__start__foo
#...

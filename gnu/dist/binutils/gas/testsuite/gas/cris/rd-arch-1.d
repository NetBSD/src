#source: arch-err-2.s
#as: --march=v32 --em=criself
#objdump: -p

#...
private flags = 3: \[symbols have a _ prefix\] \[v32\]
#pass

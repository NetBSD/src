#define a() x
a()
a ()
#define b(p) p
x/**/b(1)/**/x
x/**/b (1)/**/x
x/**/b()/**/x
#define c(p,q) p/**/q
x/**/c(1,2)/**/x
x/**/c(1)/**/x
x/**/c()/**/x

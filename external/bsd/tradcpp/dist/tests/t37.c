#define BC //**/*
#define EC */**//

BC
comment?
EC

BC comment? EC

#define FOO(x) x
FOO(abc BC def EC ghi)

#define BAR(x, y) x y
BAR(abc BC def, ghi EC jkl)

BC
#define BAZ baz
EC
BAZ

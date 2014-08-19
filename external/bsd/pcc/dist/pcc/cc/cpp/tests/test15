#define	FOO(x)	do { _foo_ ## x (); } while (/* CONSTCOND */0)
FOO(X);

#define BAR	FOO(Y)
BAR;

#define BAZ	do { _foo_(); } while (/* CONSTCOND */0)
BAZ;

#define foo(a)	(111 /*LINTED*/ a 111)
#define bar(a)	(222 a /*LINTED*/ 222)

foo(FIRST);
bar(SECOND);
foo(bar(THIRD));
bar(foo(FOURTH));

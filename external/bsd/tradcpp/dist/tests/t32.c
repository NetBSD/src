#define foo(x) "x"
#define bar(x) 'x'
#define baz frob
foo(3)
bar(3)
foo(baz)
bar(baz)
"baz"
'baz'
"foo(baz)"
"bar(baz)"

#define foo2(x) foo(x)
#define bar2(x) bar(x)
foo2(baz)
bar2(baz)

#define foo3(x) foo2(x)
#define bar3(x) bar2(x)
foo3(baz)
bar3(baz)

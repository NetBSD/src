function f(a, b, c)
{
	print match("foo", "bar", f)
}

BEGIN { f(1, 2, 3) }

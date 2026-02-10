static int x = 1;

static void
foo(void)
{
	x--;
}

static void (*fp) (void) __attribute__((__section__(".init_array"), __used__)) =
    foo;

int
main(void)
{
	return x;
}

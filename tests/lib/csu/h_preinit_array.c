static int x = 1;
static int y = 1;

static void
foo(void)
{
	x--;
}

static void
bar(void)
{
	y--;
}

static void (*fp) (void) __attribute__((__section__(".preinit_array"), __used__)) =
    foo;
static void (*bp) (void) __attribute__((__section__(".preinit_array"), __used__)) =
    bar;

int
main(void)
{
	return x + y;
}

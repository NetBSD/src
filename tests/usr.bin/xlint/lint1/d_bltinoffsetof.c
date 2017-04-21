
struct foo {
	int a;
	char *b;
};


int
main(void)
{
	return __builtin_offsetof(struct foo, b);
}

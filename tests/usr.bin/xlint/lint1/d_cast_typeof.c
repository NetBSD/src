
struct foo {
	char list;
};
struct foo *hole;

char *
foo(void)
{
	return ((char *)&((typeof(hole))0)->list);
}



struct foo {
	char list;
};
struct foo *hole;

char *
foo(void)
{
	return hole ?
	    ((char *)&((typeof(hole))0)->list) :
	    ((char *)&((typeof(*hole) *)0)->list);
}


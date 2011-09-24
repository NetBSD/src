// Test for -var-info-path-expression syntax error
// caused by PR 11912
#include <string.h>
#include <stdio.h>

class A
{
	int a;
};

class C : public A
{
	public:
		C()
		{
		};
		void testLocation()
		{
			z = 1;
		};
		int z;
};

int main()
{
	C c;
	c.testLocation();
	return 0;
}

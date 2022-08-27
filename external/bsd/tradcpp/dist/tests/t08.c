/*#include <stdio.h>*/

int d =
#if 2 > 1 ? 0 : 0 ? 1 : 1
1
#else
0
#endif
;

int e =
#if (2 > 1 ? 0 : 0) ? 1 : 1
1
#else
0
#endif
;

int f =
#if 2 > 1 ? 0 : (0 ? 1 : 1)
1
#else
0
#endif
;


int
main()
{
	int a, b, c;

	a = 2 > 1 ? 0 : 0 ? 1 : 1;
	b = (2 > 1 ? 0 : 0) ? 1 : 1;
	c = 2 > 1 ? 0 : (0 ? 1 : 1);

	printf("%d %d %d\n", a, b, c);
	printf("%d %d %d\n", d, e, f);

	return 0;
}

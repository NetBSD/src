# 2

/*
 * Between init.c 1.27 from 2015-07-28 and init.c 1.52 from 2021-01-01,
 * a bug in memberpop or pop_member led to a wrong error message
 * "undefined struct/union member: capital [101]" in the second and third
 * named initializer.
 */

struct rgb {
	unsigned red;
	unsigned green;
	unsigned blue;
};

struct hobbies {
	unsigned dancing: 1;
	unsigned running: 1;
	unsigned swimming: 1;
};

struct person {
	struct hobbies hobbies;
	struct rgb favorite_color;
};

struct city {
	struct person major;
};

struct state {
	struct city capital;
};

void func(void)
{
	struct state st = {
	    .capital.major.hobbies.dancing = 1,
	    /*
	     * Between 2015-07-28 and 2021-01-01:
	     * wrong "undefined struct/union member: capital [101]"
	     */
	    /*
	     * As of 2020-01-01:
	     * wrong "warning: bit-field initializer does not fit [180]"
	     */
	    .capital.major.favorite_color.green = 0xFF,
	    /*
	     * Between 2015-07-28 and 2021-01-01:
	     * wrong "undefined struct/union member: capital [101]"
	     */
	    /*
	     * As of 2020-01-01:
	     * wrong "warning: bit-field initializer does not fit [180]"
	     */
	    .capital.major.favorite_color.red = 0xFF
	};
}

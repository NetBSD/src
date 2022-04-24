/* $NetBSD: fmt_init.c,v 1.1 2022/04/24 10:36:37 rillig Exp $ */

/*
 * Tests for variable initializations.
 */

//indent input
int global = { initializer };
int global = {
	initializer
};

void
example(void)
{
	int local = { initializer };
	int local = {
		initializer
	};
}
//indent end

//indent run -di0
// $ XXX: The spaces around the initializer are gone.
int global = {initializer};
int global = {
	initializer
};

void
example(void)
{
	// $ XXX: The spaces around the initializer are gone.
	int local = {initializer};
	int local = {
		initializer
	};
}
//indent end

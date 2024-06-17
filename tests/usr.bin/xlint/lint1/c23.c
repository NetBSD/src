/*	$NetBSD: c23.c,v 1.16 2024/06/17 22:11:09 rillig Exp $	*/
# 3 "c23.c"

// Tests for the option -Ac23, which allows features from C23 and all earlier
// ISO standards, but none of the GNU extensions.
//
// See also:
//	c11.c
//	msg_353.c		for empty initializer braces

/* lint1-flags: -Ac23 -hw -X 351 */


int
bool_is_predefined_in_c23(void)
{
	bool t = true;
	bool f = false;
	return (t == true ? 20 : 0) + (f == false ? 3 : 0);
}

int
c99_bool_is_still_valid_in_c23(void)
{
	_Bool t = 1;
	_Bool f = 0;
	return (t == 1 ? 20 : 0) + (f == 0 ? 3 : 0);
}


bool
null_pointer_constant(const char *p, double dbl)
{
	/* expect+1: error: operands of '!=' have incompatible types 'double' and 'pointer to void' [107] */
	if (dbl != nullptr)
		p++;
	if (dbl > 0.0)
		p++;
	if (*p == '\0')
		p = nullptr;
	return p == nullptr;
}


void *
storage_class_in_compound_literal(void)
{
	typedef struct node node;
	struct node {
		node *left;
		int value;
		node *right;
	};

	node *tree;
	tree = &(static node){
	    &(static node){
		nullptr,
		3,
		nullptr,
	    },
	    5,
	    nullptr,
	};
	return tree->left;
}

int
empty_initializer_braces(void)
{
	struct s {
		int member;
	} s;

	// Empty initializer braces were introduced in C23.
	s = (struct s){};
	s = (struct s){s.member};
	return s.member;
}


_Static_assert(1 > 0, "string");
_Static_assert(1 > 0);


// The keyword 'thread_local' was introduced in C23.
thread_local int globally_visible;

// Thread-local functions don't make sense; lint allows them, though.
thread_local void
thread_local_function(void)
{
}

void
function(void)
{
	// Not sure whether it makes sense to have a function-scoped
	// thread-local variable.  Don't warn for now, let the compilers handle
	// this case.
	thread_local int function_scoped_thread_local;
}

// 'thread_local' can be combined with 'extern' and 'static', but with no other
// storage classes.  The other storage classes cannot be combined.
extern thread_local int extern_thread_local_1;
thread_local extern int extern_thread_local_2;
/* expect+1: warning: static variable 'static_thread_local_1' unused [226] */
static thread_local int static_thread_local_1;
/* expect+1: warning: static variable 'static_thread_local_2' unused [226] */
thread_local static int static_thread_local_2;


int
attributes(int i)
{
	// An attribute specifier list may be empty.
	[[]]i++;

	// There may be leading or trailing commas.
	[[,]]i++;

	// There may be arbitrary commas around or between the attributes.
	[[,,,,,]]i++;

	// An attribute may be a plain identifier without arguments.
	[[identifier]]i++;

	// The identifier may be prefixed with one additional identifier.
	[[prefix::identifier]]i++;

	// An attribute may have empty arguments.
	[[identifier()]]i++;

	// The arguments of an attribute may be arbitrary tokens.
	[[identifier([])]]i++;

	// The commas in this "argument list" are ordinary punctuator tokens,
	// they do not separate any arguments.
	// The structure of the attribute argument is:
	//	1. empty balanced token sequence between '[' and ']'
	//	2. token ','
	//	3. empty balanced token sequence between '{' and '}'
	//	4. token ','
	//	5. empty balanced token sequence between '(' and ')'
	[[identifier([], {}, ())]]i++;

	// Inside an argument, parentheses may be nested.
	[[identifier(((((())))))]]i++;
	// Inside an argument, brackets may be nested.
	[[identifier([[[[[]]]]])]]i++;
	// Inside an argument, braces may be nested.
	[[identifier({{{{{}}}}})]]i++;

	// An attribute argument may contain arbitrary punctuation.
	[[identifier(++++ ? ? ? : : :: )]]i++;

	// An attribute argument may contain constants and string literals.
	[[identifier(0, 0.0, "hello" " " "world")]]i++;

	// There may be multiple attribute specifier sequences in a row.
	[[]][[]][[]]i++;

	return i;
}

typedef int number;

void
attributes_in_parameter_declaration(
    [[maybe_unused]] int int_param,
    [[maybe_unused]] const int const_int_param,
    [[maybe_unused]] number typedef_param,
    [[maybe_unused]] const number const_typedef_param)
{
}

int
attribute_in_switch_statement(int n)
{
	switch (n) {
	case 1:
		n++;
	/* expect+1: warning: fallthrough on case statement [220] */
	case 2:
		n++;
		[[fallthrough]];
	case 3:
		n++;
		[[fallthrough]];
	default:
		n++;
	}
	return n;
}

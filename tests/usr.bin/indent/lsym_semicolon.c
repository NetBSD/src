/* $NetBSD: lsym_semicolon.c,v 1.6 2023/06/16 23:19:01 rillig Exp $ */

/*
 * Tests for the token lsym_semicolon, which represents ';' in these contexts:
 *
 * In a declaration, ';' finishes the declaration.
 *
 * In a statement, ';' finishes the statement.
 *
 * In a 'for' statement, ';' separates the 3 expressions in the head of the
 * 'for' statement.
 */

//indent input
struct {
	int member;
} global_var;
//indent end

//indent run-equals-input -di0


//indent input
void
function(void)
{
	for ( ; ; )
		stmt();
	for (;;)
		stmt();
}
//indent end

//indent run
void
function(void)
{
	for (;;)
		stmt();
	for (;;)
		stmt();
}
//indent end


//indent input
{
	switch (expr) {
// $ FIXME: Indent the 'case' at the 'switch'.
		case;
		stmt;
	case 2:
		stmt;
	}
}
//indent end

//indent run-equals-input


/*
 * A semicolon closes all possibly open '?:' expressions, so that the next ':'
 * is interpreted as a bit-field.
 */
//indent input
struct s {
	int a[len ? ? ? 1];
	int bit_field:1;
};
//indent end

//indent run-equals-input -di0


/*
 * A semicolon does not magically close any initializer braces that may still
 * be open.
 */
//indent input
int a = {{;
int b = 3;
//indent end

//indent run -di0
int a = {{;
		int b = 3;
// exit 1
// error: Standard Input:2: Stuff missing from end of file
//indent end


//indent input
{
	int a = {{;
	int b = 3;
}
//indent end

//indent run -di0
{
	int a = {{;
			int b = 3;
	}
// exit 1
// error: Standard Input:4: Stuff missing from end of file
//indent end

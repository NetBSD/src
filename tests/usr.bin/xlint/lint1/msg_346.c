/*	$NetBSD: msg_346.c,v 1.1 2021/08/09 20:07:24 rillig Exp $	*/
# 3 "msg_346.c"

// Test for message: call to '%s' effectively discards 'const' from argument [346]

char *strchr(const char *, int);

void take_const_char_ptr(const char *);
void take_char_ptr(char *);

void
example(void)
{
	const char *ccp = "const char *";
	char *cp = "char *";

	ccp = strchr(ccp, 'c');
	ccp = strchr(cp, 'c');
	/* expect+1: warning: call to 'strchr' effectively discards 'const' from argument [346] */
	cp = strchr(ccp, 'c');
	cp = strchr(cp, 'c');

	take_const_char_ptr(strchr(ccp, 'c'));
	take_const_char_ptr(strchr(cp, 'c'));
	/* expect+1: warning: call to 'strchr' effectively discards 'const' from argument [346] */
	take_char_ptr(strchr(ccp, 'c'));
	take_char_ptr(strchr(cp, 'c'));

	take_const_char_ptr(strchr("literal", 'c'));
	/* expect+1: warning: call to 'strchr' effectively discards 'const' from argument [346] */
	take_char_ptr(strchr("literal", 'c'));
}

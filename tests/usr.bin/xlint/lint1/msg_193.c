/*	$NetBSD: msg_193.c,v 1.15 2022/01/15 22:12:35 rillig Exp $	*/
# 3 "msg_193.c"

// Test for message: statement not reached [193]

/*
 * Test the reachability of statements in a function.
 *
 *	if
 *	if-else
 *	if-else-if-else
 *	for
 *	while
 *	do-while
 *	switch
 *	break
 *	continue
 *	goto
 *	return
 *
 *	constant expression
 *	system-dependent constant expression
 */

extern void reachable(void);
extern void unreachable(void);
extern _Bool maybe(void);


void
test_statement(void)
{
	reachable();
	reachable();
}

void
test_compound_statement(void)
{
	reachable();
	{
		reachable();
		reachable();
	}
	reachable();
}

void
test_if_statement(void)
{
	if (1)
		reachable();
	reachable();
	if (0)
		unreachable();		/* expect: 193 */
	reachable();
}

void
test_if_compound_statement(void)
{
	if (1) {
		reachable();
	}
	if (1) {
		{
			{
				reachable();
			}
		}
	}

	if (0) {
		unreachable();		/* expect: 193 */
	}
	if (0) {
		{
			{
				unreachable();	/* expect: 193 */
			}
		}
	}
}

void
test_if_without_else(void)
{
	if (1)
		reachable();
	reachable();

	if (0)
		unreachable();		/* expect: 193 */
	reachable();
}

void
test_if_with_else(void)
{
	if (1)
		reachable();
	else
		unreachable();		/* expect: 193 */
	reachable();

	if (0)
		unreachable();		/* expect: 193 */
	else
		reachable();
	reachable();
}

void
test_if_else_if_else(void)
{
	if (1)
		reachable();
	else if (1)			/* expect: 193 */
		unreachable();
	else
		unreachable();		/* expect: 193 */

	if (0)
		unreachable();		/* expect: 193 */
	else if (1)
		reachable();
	else
		unreachable();		/* expect: 193 */

	if (0)
		unreachable();		/* expect: 193 */
	else if (0)
		unreachable();		/* expect: 193 */
	else
		reachable();
}

void
test_if_return(void)
{
	if (1)
		return;
	unreachable();			/* expect: 193 */
}

void
test_if_else_return(void)
{
	if (1)
		reachable();
	else
		return;			/* expect: 193 */
	reachable();
}

void
test_for_forever(void)
{
	for (;;)
		reachable();
	unreachable();			/* expect: 193 */
}

void
test_for_true(void)
{
	for (; 1;)
		reachable();
	unreachable();			/* expect: 193 */
}

void
test_for_false(void)
{
	for (; 0;)
		unreachable();		/* expect: 193 */
	reachable();
}

void
test_for_break(void)
{
	for (;;) {
		reachable();
		break;
		unreachable();		/* expect: 193 */
	}
	reachable();
}

void
test_for_if_break(void)
{
	for (;;) {
		reachable();
		if (0) {
			unreachable();	/* expect: 193 */
			break;
			unreachable();	/* expect: 193 */
		}
		if (1) {
			reachable();
			break;
			unreachable();	/* expect: 193 */
		}
		unreachable();		/* expect: 193 */
	}
	reachable();
}

void
test_for_continue(void)
{
	for (;;) {
		reachable();
		continue;
		unreachable();		/* expect: 193 */
	}
	unreachable();			/* expect: 193 */
}

void
test_for_if_continue(void)
{
	for (;;) {
		reachable();
		if (0) {
			unreachable();	/* expect: 193 */
			continue;
			unreachable();	/* expect: 193 */
		}
		if (1) {
			reachable();
			continue;
			unreachable();	/* expect: 193 */
		}
		unreachable();		/* expect: 193 */
	}
	unreachable();			/* expect: 193 */
}

void
test_for_return(void)
{
	for (;;) {
		reachable();
		return;
		unreachable();		/* expect: 193 */
	}
	unreachable();			/* expect: 193 */
}

void
test_for_if_return(void)
{
	for (;;) {
		reachable();
		if (0) {
			unreachable();	/* expect: 193 */
			return;
			unreachable();	/* expect: 193 */
		}
		if (1) {
			reachable();
			return;
			unreachable();	/* expect: 193 */
		}
		unreachable();		/* expect: 193 */
	}
	unreachable();			/* expect: 193 */
}

void
test_while_true(void)
{
	while (1)
		reachable();
	unreachable();			/* expect: 193 */
}

void
test_while_false(void)
{
	while (0)
		unreachable();		/* expect: 193 */
	reachable();
}

void
test_while_break(void)
{
	while (1) {
		reachable();
		break;
		unreachable();		/* expect: 193 */
	}
	reachable();
}

void
test_while_if_break(void)
{
	while (1) {
		reachable();
		if (0) {
			unreachable();	/* expect: 193 */
			break;
			unreachable();	/* expect: 193 */
		}
		if (1) {
			reachable();
			break;
			unreachable();	/* expect: 193 */
		}
		unreachable();		/* expect: 193 */
	}
	reachable();
}

void
test_while_continue(void)
{
	while (1) {
		reachable();
		continue;
		unreachable();		/* expect: 193 */
	}
	unreachable();			/* expect: 193 */
}

void
test_while_if_continue(void)
{
	while (1) {
		reachable();
		if (0) {
			unreachable();	/* expect: 193 */
			continue;
			unreachable();	/* expect: 193 */
		}
		if (1) {
			reachable();
			continue;
			unreachable();	/* expect: 193 */
		}
		unreachable();		/* expect: 193 */
	}
	unreachable();			/* expect: 193 */
}

void
test_while_return(void)
{
	while (1) {
		reachable();
		return;
		unreachable();		/* expect: 193 */
	}
	unreachable();			/* expect: 193 */
}

void
test_while_if_return(void)
{
	while (1) {
		reachable();
		if (0) {
			unreachable();	/* expect: 193 */
			return;
			unreachable();	/* expect: 193 */
		}
		if (1) {
			reachable();
			return;
			unreachable();	/* expect: 193 */
		}
		unreachable();		/* expect: 193 */
	}
	unreachable();			/* expect: 193 */
}

void
test_do_while_true(void)
{
	do {
		reachable();
	} while (1);
	unreachable();			/* expect: 193 */
}

void
test_do_while_false(void)
{
	do {
		reachable();
	} while (0);
	reachable();
}

void
test_do_while_break(void)
{
	do {
		reachable();
		break;
		unreachable();		/* expect: 193 */
	} while (1);
	reachable();
}

void
test_do_while_if_break(void)
{
	do {
		reachable();
		if (0) {
			unreachable();	/* expect: 193 */
			break;
			unreachable();	/* expect: 193 */
		}
		if (1) {
			reachable();
			break;
			unreachable();	/* expect: 193 */
		}
		unreachable();		/* expect: 193 */
	} while (1);
	reachable();
}

void
test_do_while_continue(void)
{
	do {
		reachable();
		continue;
		unreachable();		/* expect: 193 */
	} while (1);
	unreachable();			/* expect: 193 */
}

void
test_do_while_if_continue(void)
{
	do {
		reachable();
		if (0) {
			unreachable();	/* expect: 193 */
			continue;
			unreachable();	/* expect: 193 */
		}
		if (1) {
			reachable();
			continue;
			unreachable();	/* expect: 193 */
		}
		unreachable();		/* expect: 193 */
	} while (1);
	unreachable();			/* expect: 193 */
}

void
test_do_while_return(void)
{
	do {
		reachable();
		return;
		unreachable();		/* expect: 193 */
	} while (1);
	unreachable();			/* expect: 193 */
}

void
test_do_while_if_return(void)
{
	do {
		reachable();
		if (0) {
			unreachable();	/* expect: 193 */
			return;
			unreachable();	/* expect: 193 */
		}
		if (1) {
			reachable();
			return;
			unreachable();	/* expect: 193 */
		}
		unreachable();		/* expect: 193 */
	} while (1);
	unreachable();			/* expect: 193 */
}

void
test_if_nested(void)
{
	if (0) {
		if (1)			/* expect: 193 */
			unreachable();
		else
			unreachable();	/* expect: 193 *//* XXX: redundant */

		if (0)
			unreachable();	/* expect: 193 *//* XXX: redundant */
		else
			unreachable();

		unreachable();
	}
	reachable();

	if (1) {
		if (1)
			reachable();
		else
			unreachable();	/* expect: 193 */

		if (0)
			unreachable();	/* expect: 193 */
		else
			reachable();

		reachable();
	}
	reachable();
}

void
test_if_maybe(void)
{
	if (maybe()) {
		if (0)
			unreachable();	/* expect: 193 */
		else
			reachable();
		reachable();
	}
	reachable();

	if (0) {
		if (maybe())		/* expect: 193 */
			unreachable();
		else
			unreachable();
		unreachable();
	}
	reachable();

	if (1) {
		if (maybe())
			reachable();
		else
			reachable();
		reachable();
	}
	reachable();
}

/*
 * To compute the reachability graph of this little monster, lint would have
 * to keep all statements and their relations from the whole function in
 * memory.  It doesn't do that.  Therefore it does not warn about any
 * unreachable statements in this function.
 */
void
test_goto_numbers_alphabetically(void)
{
	goto one;
eight:
	goto nine;
five:
	return;
four:
	goto five;
nine:
	goto ten;
one:
	goto two;
seven:
	goto eight;
six:				/* expect: warning: label 'six' unused */
	goto seven;
ten:
	return;
three:
	goto four;
two:
	goto three;
}

void
test_while_goto(void)
{
	while (1) {
		goto out;
		break;		/* lint only warns with the -b option */
	}
	unreachable();		/* expect: 193 */
out:
	reachable();
}

void
test_unreachable_label(void)
{
	if (0)
		goto unreachable;	/* expect: 193 */
	goto reachable;

	/* named_label assumes that any label is reachable. */
unreachable:
	unreachable();
reachable:
	reachable();
}

/* TODO: switch */

/* TODO: system-dependent constant expression (see tn_system_dependent) */

void suppressed(void);

void
lint_annotation_NOTREACHED(void)
{
	if (0) {
		/* expect+1: warning: statement not reached [193] */
		unreachable();
	}

	if (0) {
		/* NOTREACHED */
		suppressed();
	}

	if (0)
		/* NOTREACHED */
		suppressed();

	if (1) {
		reachable();
	}

	if (1) {
		/* NOTREACHED */
		suppressed();
	}

	/*
	 * Since the condition in the 'if' statement is constant, lint knows
	 * that the branch is unconditionally taken.  The annotation comment
	 * marks that branch as not reached, which means that any following
	 * statement cannot be reached as well.
	 */
	/* expect+1: warning: statement not reached [193] */
	if (1)
		/* NOTREACHED */
		suppressed();
}

/*
 * Since at least 2002, lint does not detect a double semicolon.  See
 * cgram.y, expression_statement, T_SEMI.
 */
int
test_empty_statement(int x)
{
	/* TODO: expect+1: warning: statement not reachable [193] */
	return x > 0 ? x : -x;;
}

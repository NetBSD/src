/*	$NetBSD: d_c99_compound_literal_comma.c,v 1.3 2021/03/20 11:24:49 rillig Exp $	*/
# 3 "d_c99_compound_literal_comma.c"

/*-
 * Ensure that compound literals can be parsed.
 *
 * C99 6.5.2 "Postfix operators" for the syntax.
 * C99 6.5.2.5 "Compound literals" for the semantics.
 */

struct point {
	int x;
	int y;
};

struct point
point_abs(struct point point)
{
	/* No designators, no trailing comma. */
	if (point.x >= 0 && point.y >= 0)
		return (struct point){ point.x, point.y };

	/* Designators, no trailing comma. */
	if (point.x >= 0)
		return (struct point){ .x = point.x, .y = -point.y };

	/* No designators, trailing comma. */
	if (point.y >= 0)
		return (struct point){ point.x, point.y, };

	/* Designators, trailing comma. */
	return (struct point){ .x = point.x, .y = -point.y, };
}

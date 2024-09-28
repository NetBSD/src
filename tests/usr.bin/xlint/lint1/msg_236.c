/*	$NetBSD: msg_236.c,v 1.7 2024/09/28 16:09:23 rillig Exp $	*/
# 3 "msg_236.c"

// Test for message: static function '%s' unused [236]

/* lint1-extra-flags: -X 351 */

void
external_function(void)
{
}

/* expect+2: warning: static function 'static_function' unused [236] */
static void
static_function(void)
{
}

static inline void
inline_function(void)
{
}

__attribute__((__constructor__))
static void
constructor_function(void)
{
}

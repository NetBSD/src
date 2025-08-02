/*	$NetBSD: msg_236.c,v 1.5.2.1 2025/08/02 05:58:17 perseant Exp $	*/
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

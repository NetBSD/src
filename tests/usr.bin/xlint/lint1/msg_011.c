/*	$NetBSD: msg_011.c,v 1.2 2021/01/07 18:37:41 rillig Exp $	*/
# 3 "msg_011.c"

// Test for message: bit-field initializer out of range [11]

struct example {
	int bit_field: 4;
} example_var[] = {
    { 2 },
    { 1 + 1 },
    { 1ULL << 40 },
    { 1LL << 40 },
    { -16 - 1 },
};

struct example_u {
	unsigned int bit_field: 4;
} example_var_u[] = {
    { 2 },
    { 1 + 1 },
    { 1ULL << 40 },
    { 1LL << 40 },
    { -16 - 1 },
};

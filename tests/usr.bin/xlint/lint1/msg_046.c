/*	$NetBSD: msg_046.c,v 1.5 2022/06/11 11:52:13 rillig Exp $	*/
# 3 "msg_046.c"

// Test for message: %s tag '%s' redeclared as %s [46]

/* expect+1: warning: struct 'tag1' never defined [233] */
struct tag1;
/* expect+2: error: struct tag 'tag1' redeclared as union [46] */
/* expect+1: warning: union 'tag1' never defined [234] */
union tag1;

/* expect+1: warning: union 'tag2' never defined [234] */
union tag2;
/* expect+2: error: union tag 'tag2' redeclared as enum [46] */
/* expect+1: warning: enum 'tag2' never defined [235] */
enum tag2;

/* expect+1: warning: enum 'tag3' never defined [235] */
enum tag3;
/* expect+2: error: enum tag 'tag3' redeclared as struct [46] */
/* expect+1: warning: struct 'tag3' never defined [233] */
struct tag3;

/* expect+2: error: union tag 'tag1' redeclared as struct [46] */
/* expect+1: warning: struct 'tag1' never defined [233] */
struct tag1 *use_tag1(void);
/* expect+2: error: enum tag 'tag2' redeclared as union [46] */
/* expect+1: warning: union 'tag2' never defined [234] */
union tag2 *use_tag2(void);
/* expect+2: error: struct tag 'tag3' redeclared as enum [46] */
/* expect+1: warning: enum 'tag3' never defined [235] */
enum tag3 *use_tag3(void);

/* expect+2: error: struct tag 'tag1' redeclared as union [46] */
/* expect+1: warning: union 'tag1' never defined [234] */
union tag1 *mismatch_tag1(void);

/*
  Copyright (c) 2010 Alistair Crooks (agc@pkgsrc.org)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.
*/
#ifndef MJ_H_
#define MJ_H_	20100718

enum {
	MJ_NULL		= 1,
	MJ_FALSE	= 2,
	MJ_TRUE		= 3,
	MJ_NUMBER	= 4,
	MJ_STRING	= 5,
	MJ_ARRAY	= 6,
	MJ_OBJECT	= 7
};

/* a minimalist JSON node */
typedef struct mj_t {
	unsigned	type;		/* type of JSON node */
	unsigned	c;		/* # of chars */
	unsigned	size;		/* size of array */
	union {
		struct mj_t	*v;	/* sub-objects */
		char		*s;	/* string value */
	} value;
} mj_t;

/* creation and deletion */
int mj_create(mj_t *, const char *, ...);
int mj_parse(mj_t *, const char *, int *, int *, int *);
int mj_append(mj_t *, const char *, ...);
int mj_append_field(mj_t *, const char *, const char *, ...);
int mj_deepcopy(mj_t *, mj_t *);
void mj_delete(mj_t *);

/* JSON object access */
int mj_arraycount(mj_t *);
mj_t *mj_object_find(mj_t *, const char *);
mj_t *mj_get_atom(mj_t *, ...);

/* textual output */
int mj_snprint(char *, size_t, mj_t *);
int mj_asprint(char **, mj_t *);
int mj_string_size(mj_t *);

#endif

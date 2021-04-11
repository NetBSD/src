/*	$NetBSD: str.h,v 1.1 2021/04/11 12:06:53 rillig Exp $	*/

/*
 Copyright (c) 2021 Roland Illig <rillig@NetBSD.org>
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions
 are met:

 1. Redistributions of source code must retain the above copyright
    notice, this list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright
    notice, this list of conditions and the following disclaimer in the
    documentation and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 POSSIBILITY OF SUCH DAMAGE.
 */


/*
 * Memory-efficient string handling.
 */


/* A read-only string that may need to be freed after use. */
typedef struct FStr {
	const char *str;
	void *freeIt;
} FStr;

/* A modifiable string that may need to be freed after use. */
typedef struct MFStr {
	char *str;
	void *freeIt;
} MFStr;

/* A read-only range of a character array, NOT null-terminated. */
typedef struct {
	const char *start;
	const char *end;
} Substring;

/*
 * Builds a string, only allocating memory if the string is different from the
 * expected string.
 */
typedef struct LazyBuf {
	char *data;
	size_t len;
	size_t cap;
	Substring expected;
	void *freeIt;
} LazyBuf;

/* The result of splitting a string into words. */
typedef struct Words {
	char **words;
	size_t len;
	void *freeIt;
} Words;


MAKE_INLINE FStr
FStr_Init(const char *str, void *freeIt)
{
	FStr fstr;
	fstr.str = str;
	fstr.freeIt = freeIt;
	return fstr;
}

/* Return a string that is the sole owner of str. */
MAKE_INLINE FStr
FStr_InitOwn(char *str)
{
	return FStr_Init(str, str);
}

/* Return a string that refers to the shared str. */
MAKE_INLINE FStr
FStr_InitRefer(const char *str)
{
	return FStr_Init(str, NULL);
}

MAKE_INLINE void
FStr_Done(FStr *fstr)
{
	free(fstr->freeIt);
#ifdef CLEANUP
	fstr->str = NULL;
	fstr->freeIt = NULL;
#endif
}


MAKE_INLINE MFStr
MFStr_Init(char *str, void *freeIt)
{
	MFStr mfstr;
	mfstr.str = str;
	mfstr.freeIt = freeIt;
	return mfstr;
}

/* Return a string that is the sole owner of str. */
MAKE_INLINE MFStr
MFStr_InitOwn(char *str)
{
	return MFStr_Init(str, str);
}

/* Return a string that refers to the shared str. */
MAKE_INLINE MFStr
MFStr_InitRefer(char *str)
{
	return MFStr_Init(str, NULL);
}

MAKE_INLINE void
MFStr_Done(MFStr *mfstr)
{
	free(mfstr->freeIt);
#ifdef CLEANUP
	mfstr->str = NULL;
	mfstr->freeIt = NULL;
#endif
}


MAKE_INLINE Substring
Substring_Init(const char *start, const char *end)
{
	Substring sub;

	sub.start = start;
	sub.end = end;
	return sub;
}

MAKE_INLINE Substring
Substring_InitStr(const char *str)
{
	return Substring_Init(str, str + strlen(str));
}

MAKE_INLINE size_t
Substring_Length(Substring sub)
{
	return (size_t)(sub.end - sub.start);
}

MAKE_INLINE bool
Substring_Equals(Substring sub, const char *str)
{
	size_t len = strlen(str);
	return Substring_Length(sub) == len &&
	       memcmp(sub.start, str, len) == 0;
}

MAKE_INLINE Substring
Substring_Sub(Substring sub, size_t start, size_t end)
{
	assert(start <= Substring_Length(sub));
	assert(end <= Substring_Length(sub));
	return Substring_Init(sub.start + start, sub.start + end);
}

MAKE_INLINE FStr
Substring_Str(Substring sub)
{
	return FStr_InitOwn(bmake_strsedup(sub.start, sub.end));
}


MAKE_INLINE void
LazyBuf_Init(LazyBuf *buf, Substring expected)
{
	buf->data = NULL;
	buf->len = 0;
	buf->cap = 0;
	buf->expected = expected;
	buf->freeIt = NULL;
}

MAKE_INLINE void
LazyBuf_Done(LazyBuf *buf)
{
	free(buf->freeIt);
}

MAKE_INLINE void
LazyBuf_Add(LazyBuf *buf, char ch)
{

	if (buf->data != NULL) {
		if (buf->len == buf->cap) {
			buf->cap *= 2;
			buf->data = bmake_realloc(buf->data, buf->cap);
		}
		buf->data[buf->len++] = ch;

	} else if (buf->len < Substring_Length(buf->expected) &&
	    ch == buf->expected.start[buf->len]) {
		buf->len++;
		return;

	} else {
		buf->cap = buf->len + 16;
		buf->data = bmake_malloc(buf->cap);
		memcpy(buf->data, buf->expected.start, buf->len);
		buf->data[buf->len++] = ch;
	}
}

MAKE_INLINE void
LazyBuf_AddStr(LazyBuf *buf, const char *str)
{
	const char *p;

	for (p = str; *p != '\0'; p++)
		LazyBuf_Add(buf, *p);
}

MAKE_INLINE Substring
LazyBuf_Get(const LazyBuf *buf)
{
	const char *start = buf->data != NULL
	    ? buf->data : buf->expected.start;
	return Substring_Init(start, start + buf->len);
}

MAKE_INLINE FStr
LazyBuf_DoneGet(LazyBuf *buf)
{
	if (buf->data != NULL) {
		LazyBuf_Add(buf, '\0');
		return FStr_InitOwn(buf->data);
	}
	return Substring_Str(LazyBuf_Get(buf));
}


Words Str_Words(const char *, bool);

MAKE_INLINE void
Words_Free(Words w)
{
	free(w.words);
	free(w.freeIt);
}


char *str_concat2(const char *, const char *);
char *str_concat3(const char *, const char *, const char *);
char *str_concat4(const char *, const char *, const char *, const char *);

bool Str_Match(const char *, const char *);

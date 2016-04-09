/*	$NetBSD: chartype.h,v 1.27 2016/04/09 18:43:17 christos Exp $	*/

/*-
 * Copyright (c) 2009 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _h_chartype_f
#define _h_chartype_f

#ifndef NARROWCHAR

/* Ideally we should also test the value of the define to see if it
 * supports non-BMP code points without requiring UTF-16, but nothing
 * seems to actually advertise this properly, despite Unicode 3.1 having
 * been around since 2001... */
#if !defined(__NetBSD__) && !defined(__sun) && !(defined(__APPLE__) && defined(__MACH__)) && !defined(__OpenBSD__) && !defined(__FreeBSD__)
#ifndef __STDC_ISO_10646__
/* In many places it is assumed that the first 127 code points are ASCII
 * compatible, so ensure wchar_t indeed does ISO 10646 and not some other
 * funky encoding that could break us in weird and wonderful ways. */
	#error wchar_t must store ISO 10646 characters
#endif
#endif

/* Oh for a <uchar.h> with char32_t and __STDC_UTF_32__ in it...
 * ref: ISO/IEC DTR 19769
 */
#if WCHAR_MAX < INT32_MAX
#warning Build environment does not support non-BMP characters
#endif

#define Char			wchar_t
#define FUN(prefix,rest)	prefix ## _w ## rest
#define FUNW(type)		type ## _w
#define TYPE(type)		type ## W
#define STR(x)			L ## x

#define Strlen(x)       wcslen(x)
#define Strchr(s,c)     wcschr(s,c)
#define Strdup(x)       wcsdup(x)
#define Strncpy(d,s,n)  wcsncpy(d,s,n)
#define Strncat(d,s,n)  wcsncat(d,s,n)
#define Strcmp(s,v)     wcscmp(s,v)
#define Strncmp(s,v,n)  wcsncmp(s,v,n)

#else /* NARROW */

#define Char			char
#define FUN(prefix,rest)	prefix ## _ ## rest
#define FUNW(type)		type
#define TYPE(type)		type
#define STR(x)			x

#define Strlen(x)       strlen(x)
#define Strchr(s,c)     strchr(s,c)
#define Strdup(x)       strdup(x)
#define Strncpy(d,s,n)  strncpy(d,s,n)
#define Strncat(d,s,n)  strncat(d,s,n)

#define Strcmp(s,v)     strcmp(s,v)
#define Strncmp(s,v,n)  strncmp(s,v,n)
#endif


#ifndef NARROWCHAR
/*
 * Conversion buffer
 */
typedef struct ct_buffer_t {
        char    *cbuff;
        size_t  csize;
        Char *wbuff;
        size_t  wsize;
} ct_buffer_t;

#define ct_encode_string __ct_encode_string
/* Encode a wide-character string and return the UTF-8 encoded result. */
public char *ct_encode_string(const Char *, ct_buffer_t *);

#define ct_decode_string __ct_decode_string
/* Decode a (multi)?byte string and return the wide-character string result. */
public Char *ct_decode_string(const char *, ct_buffer_t *);

/* Decode a (multi)?byte argv string array.
 * The pointer returned must be free()d when done. */
protected Char **ct_decode_argv(int, const char *[],  ct_buffer_t *);

/* Resizes the conversion buffer(s) if needed. */
protected int ct_conv_cbuff_resize(ct_buffer_t *, size_t);
protected int ct_conv_wbuff_resize(ct_buffer_t *, size_t);
protected ssize_t ct_encode_char(char *, size_t, Char);
protected size_t ct_enc_width(Char);

#else
#define	ct_encode_string(s, b)	(s)
#define ct_decode_string(s, b)	(s)
#endif

#ifndef NARROWCHAR
/* Encode a characted into the destination buffer, provided there is sufficent
 * buffer space available. Returns the number of bytes used up (zero if the
 * character cannot be encoded, -1 if there was not enough space available). */

/* The maximum buffer size to hold the most unwieldly visual representation,
 * in this case \U+nnnnn. */
#define VISUAL_WIDTH_MAX ((size_t)8)

/* The terminal is thought of in terms of X columns by Y lines. In the cases
 * where a wide character takes up more than one column, the adjacent
 * occupied column entries will contain this faux character. */
#define MB_FILL_CHAR ((Char)-1)

/* Visual width of character c, taking into account ^? , \0177 and \U+nnnnn
 * style visual expansions. */
protected int ct_visual_width(Char);

/* Turn the given character into the appropriate visual format, matching
 * the width given by ct_visual_width(). Returns the number of characters used
 * up, or -1 if insufficient space. Buffer length is in count of Char's. */
protected ssize_t ct_visual_char(Char *, size_t, Char);

/* Convert the given string into visual format, using the ct_visual_char()
 * function. Uses a static buffer, so not threadsafe. */
protected const Char *ct_visual_string(const Char *);


/* printable character, use ct_visual_width() to find out display width */
#define CHTYPE_PRINT        ( 0)
/* control character found inside the ASCII portion of the charset */
#define CHTYPE_ASCIICTL     (-1)
/* a \t */
#define CHTYPE_TAB          (-2)
/* a \n */
#define CHTYPE_NL           (-3)
/* non-printable character */
#define CHTYPE_NONPRINT     (-4)
/* classification of character c, as one of the above defines */
protected int ct_chr_class(Char c);
#endif

#endif /* _chartype_f */

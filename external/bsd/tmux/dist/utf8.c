/* $OpenBSD$ */

/*
 * Copyright (c) 2008 Nicholas Marriott <nicm@users.sourceforge.net>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF MIND, USE, DATA OR PROFITS, WHETHER
 * IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <sys/types.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include "tmux.h"

struct utf8_width_entry {
	u_int	first;
	u_int	last;

/* Set a single character. */
void
utf8_set(struct utf8_data *ud, u_char ch)
{
	static const struct utf8_data empty = { { 0 }, 1, 1, 1 };

	memcpy(ud, &empty, sizeof *ud);
	*ud->data = ch;
}

/* Copy UTF-8 character. */
void
utf8_set(struct utf8_data *utf8data, u_char ch)
{
	*utf8data->data = ch;
	utf8data->size = 1;

	utf8data->width = 1;
}

/*
 * Open UTF-8 sequence.
 *
 * 11000010-11011111 C2-DF start of 2-byte sequence
 * 11100000-11101111 E0-EF start of 3-byte sequence
 * 11110000-11110100 F0-F4 start of 4-byte sequence
 *
 * Returns 1 if more UTF-8 to come, 0 if not UTF-8.
 */
int
utf8_open(struct utf8_data *utf8data, u_char ch)
{
	memset(utf8data, 0, sizeof *utf8data);
	if (ch >= 0xc2 && ch <= 0xdf)
		utf8data->size = 2;
	else if (ch >= 0xe0 && ch <= 0xef)
		utf8data->size = 3;
	else if (ch >= 0xf0 && ch <= 0xf4)
		utf8data->size = 4;
	else
		return (0);
	utf8_append(utf8data, ch);
	return (1);
}

/*
 * Append character to UTF-8, closing if finished.
 *
 * Returns 1 if more UTF-8 data to come, 0 if finished.
 */
int
utf8_append(struct utf8_data *utf8data, u_char ch)
{
	if (utf8data->have >= utf8data->size)
		fatalx("UTF-8 character overflow");
	if (utf8data->size > sizeof utf8data->data)
		fatalx("UTF-8 character size too large");

	utf8data->data[utf8data->have++] = ch;
	if (utf8data->have != utf8data->size)
		return (1);

	utf8data->width = utf8_width(utf8data);
	return (0);
}

/* Check if two width tree entries overlap. */
int
utf8_overlap(struct utf8_width_entry *item1, struct utf8_width_entry *item2)
{
	if (item1->first >= item2->first && item1->first <= item2->last)
		return (1);
	if (item1->last >= item2->first && item1->last <= item2->last)
		return (1);
	if (item2->first >= item1->first && item2->first <= item1->last)
		return (1);
	if (item2->last >= item1->first && item2->last <= item1->last)
		return (1);
	return (0);
}

/* Build UTF-8 width tree. */
void
utf8_build(void)
{
	struct utf8_width_entry	**ptr, *item, *node;
	u_int			  i, j;

	for (i = 0; i < nitems(utf8_width_table); i++) {
		item = &utf8_width_table[i];

		for (j = 0; j < nitems(utf8_width_table); j++) {
			if (i != j && utf8_overlap(item, &utf8_width_table[j]))
				log_fatalx("utf8 overlap: %u %u", i, j);
		}

		ptr = &utf8_width_root;
		while (*ptr != NULL) {
			node = *ptr;
			if (item->last < node->first)
				ptr = &node->left;
			else if (item->first > node->last)
				ptr = &node->right;
		}
		*ptr = item;
	}
}

/* Combine UTF-8 into 32-bit Unicode. */
u_int
utf8_combine(const struct utf8_data *utf8data)
{
	u_int	value;

#ifdef HAVE_UTF8PROC
	width = utf8proc_wcwidth(wc);
#else
	width = wcwidth(wc);
#endif
	if (width < 0 || width > 0xff) {
		log_debug("Unicode %04lx, wcwidth() %d", (long)wc, width);

#ifndef __OpenBSD__
		/*
		 * Many platforms (particularly and inevitably OS X) have no
		 * width for relatively common characters (wcwidth() returns
		 * -1); assume width 1 in this case. This will be wrong for
		 * genuinely nonprintable characters, but they should be
		 * rare. We may pass through stuff that ideally we would block,
		 * but this is no worse than sending the same to the terminal
		 * without tmux.
		 */
		if (width < 0)
			return (1);
#endif
		return (-1);
	}
	return (width);
}

/* Combine UTF-8 into Unicode. */
enum utf8_state
utf8_combine(const struct utf8_data *ud, wchar_t *wc)
{
#ifdef HAVE_UTF8PROC
	switch (utf8proc_mbtowc(wc, ud->data, ud->size)) {
#else
	switch (mbtowc(wc, (const char *)ud->data, ud->size)) {
#endif
	case -1:
		log_debug("UTF-8 %.*s, mbtowc() %d", (int)ud->size, ud->data,
		    errno);
		mbtowc(NULL, NULL, MB_CUR_MAX);
		return (UTF8_ERROR);
	case 0:
		return (UTF8_ERROR);
	default:
		return (UTF8_DONE);
	}
	ptr[0] = uc;
	return (1);
}

/* Split Unicode into UTF-8. */
enum utf8_state
utf8_split(wchar_t wc, struct utf8_data *ud)
{
	char	s[MB_LEN_MAX];
	int	slen;

#ifdef HAVE_UTF8PROC
	slen = utf8proc_wctomb(s, wc);
#else
	slen = wctomb(s, wc);
#endif
	if (slen <= 0 || slen > (int)sizeof ud->data)
		return (UTF8_ERROR);

	value = utf8_combine(utf8data);

	item = utf8_width_root;
	while (item != NULL) {
		if (value < item->first)
			item = item->left;
		else if (value > item->last)
			item = item->right;
		else
			return (item->width);
	}
	return (1);
}

/*
 * Encode len characters from src into dst, which is guaranteed to have four
 * bytes available for each character from src (for \abc or UTF-8) plus space
 * for \0.
 */
int
utf8_strvis(char *dst, const char *src, size_t len, int flag)
{
	struct utf8_data	 utf8data;
	const char		*start, *end;
	int			 more;
	size_t			 i;

	start = dst;
	end = src + len;

	while (src < end) {
		if (utf8_open(&utf8data, *src)) {
			more = 1;
			while (++src < end && more)
				more = utf8_append(&utf8data, *src);
			if (!more) {
				/* UTF-8 character finished. */
				for (i = 0; i < utf8data.size; i++)
					*dst++ = utf8data.data[i];
				continue;
			} else if (utf8data.have > 0) {
				/* Not a complete UTF-8 character. */
				src -= utf8data.have;
			}
		}
		if (src < end - 1)
			dst = vis(dst, src[0], flag, src[1]);
		else if (src < end)
			dst = vis(dst, src[0], flag, '\0');
		src++;
	}

	*dst = '\0';
	return (dst - start);
}

/* Same as utf8_strvis but allocate the buffer. */
int
utf8_stravis(char **dst, const char *src, int flag)
{
	char	*buf;
	int	 len;

	buf = xreallocarray(NULL, 4, strlen(src) + 1);
	len = utf8_strvis(buf, src, strlen(src), flag);

	*dst = xrealloc(buf, len + 1);
	return (len);
}

/*
 * Sanitize a string, changing any UTF-8 characters to '_'. Caller should free
 * the returned string. Anything not valid printable ASCII or UTF-8 is
 * stripped.
 */
char *
utf8_sanitize(const char *src)
{
	char			*dst;
	size_t			 n;
	enum utf8_state		 more;
	struct utf8_data	 ud;
	u_int			 i;

	dst = NULL;

	n = 0;
	while (*src != '\0') {
		dst = xreallocarray(dst, n + 1, sizeof *dst);
		if ((more = utf8_open(&ud, *src)) == UTF8_MORE) {
			while (*++src != '\0' && more == UTF8_MORE)
				more = utf8_append(&ud, *src);
			if (more == UTF8_DONE) {
				dst = xreallocarray(dst, n + ud.width,
				    sizeof *dst);
				for (i = 0; i < ud.width; i++)
					dst[n++] = '_';
				continue;
			}
			src -= ud.have;
		}
		if (*src > 0x1f && *src < 0x7f)
			dst[n++] = *src;
		else
			dst[n++] = '_';
		src++;
	}

	dst = xreallocarray(dst, n + 1, sizeof *dst);
	dst[n] = '\0';
	return (dst);
}

/* Get UTF-8 buffer length. */
size_t
utf8_strlen(const struct utf8_data *s)
{
	size_t	i;

	for (i = 0; s[i].size != 0; i++)
		/* nothing */;
	return (i);
}

/* Get UTF-8 string width. */
u_int
utf8_strwidth(const struct utf8_data *s, ssize_t n)
{
	ssize_t	i;
	u_int	width;

	width = 0;
	for (i = 0; s[i].size != 0; i++) {
		if (n != -1 && n == i)
			break;
		width += s[i].width;
	}
	return (width);
}

/*
 * Convert a string into a buffer of UTF-8 characters. Terminated by size == 0.
 * Caller frees.
 */
struct utf8_data *
utf8_fromcstr(const char *src)
{
	struct utf8_data	*dst;
	size_t			 n;
	int			 more;

	dst = NULL;

	n = 0;
	while (*src != '\0') {
		dst = xreallocarray(dst, n + 1, sizeof *dst);
		if (utf8_open(&dst[n], *src)) {
			more = 1;
			while (*++src != '\0' && more)
				more = utf8_append(&dst[n], *src);
			if (!more) {
				n++;
				continue;
			}
			src -= dst[n].have;
		}
		utf8_set(&dst[n], *src);
		src++;

		n++;
	}

	dst = xreallocarray(dst, n + 1, sizeof *dst);
	dst[n].size = 0;
	return (dst);
}

/* Convert from a buffer of UTF-8 characters into a string. Caller frees. */
char *
utf8_tocstr(struct utf8_data *src)
{
	char	*dst;
	size_t	 n;

	dst = NULL;

	n = 0;
	for(; src->size != 0; src++) {
		dst = xreallocarray(dst, n + src->size, 1);
		memcpy(dst + n, src->data, src->size);
		n += src->size;
	}

	dst = xreallocarray(dst, n + 1, 1);
	dst[n] = '\0';
	return (dst);
}

/* Get width of UTF-8 string. */
u_int
utf8_cstrwidth(const char *s)
{
	struct utf8_data	tmp;
	u_int			width;
	int			more;

	width = 0;
	while (*s != '\0') {
		if (utf8_open(&tmp, *s)) {
			more = 1;
			while (*++s != '\0' && more)
				more = utf8_append(&tmp, *s);
			if (!more) {
				width += tmp.width;
				continue;
			}
			s -= tmp.have;
		}
		width++;
		s++;
	}
	return (width);
}

/* Trim UTF-8 string to width. Caller frees. */
char *
utf8_trimcstr(const char *s, u_int width)
{
	struct utf8_data	*tmp, *next;
	char			*out;
	u_int			 at;

	tmp = utf8_fromcstr(s);

	at = 0;
	for (next = tmp; next->size != 0; next++) {
		if (at + next->width > width) {
			next->size = 0;
			break;
		}
		at += next->width;
	}

	out = utf8_tocstr(tmp);
	free(tmp);
	return (out);
}

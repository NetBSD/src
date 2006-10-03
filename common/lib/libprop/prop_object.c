/*	$NetBSD: prop_object.c,v 1.6 2006/10/03 15:45:04 thorpej Exp $	*/

/*-
 * Copyright (c) 2006 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * This code is derived from software contributed to The NetBSD Foundation
 * by Jason R. Thorpe.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the NetBSD
 *      Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
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

#include <prop/prop_object.h>
#include "prop_object_impl.h"

#if !defined(_KERNEL) && !defined(_STANDALONE)
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#endif

#ifdef _STANDALONE
void *
_prop_standalone_calloc(size_t size)
{
	void *rv;

	rv = alloc(size);
	if (rv != NULL)
		memset(rv, 0, size);

	return (rv);
}

void *
_prop_standalone_realloc(void *v, size_t size)
{
	void *rv;

	rv = alloc(size);
	if (rv != NULL) {
		memcpy(rv, v, size);	/* XXX */
		dealloc(v, 0);		/* XXX */
	}
	
	return (rv);
}
#endif /* _STANDALONE */

/*
 * _prop_object_init --
 *	Initialize an object.  Called when sub-classes create
 *	an instance.
 */
void
_prop_object_init(struct _prop_object *po, const struct _prop_object_type *pot)
{

	po->po_type = pot;
	po->po_refcnt = 1;
}

/*
 * _prop_object_fini --
 *	Finalize an object.  Called when sub-classes destroy
 *	an instance.
 */
/*ARGSUSED*/
void
_prop_object_fini(struct _prop_object *po)
{
	/* Nothing to do, currently. */
}

/*
 * _prop_object_externalize_start_tag --
 *	Append an XML-style start tag to the externalize buffer.
 */
boolean_t
_prop_object_externalize_start_tag(
    struct _prop_object_externalize_context *ctx, const char *tag)
{
	unsigned int i;

	for (i = 0; i < ctx->poec_depth; i++) {
		if (_prop_object_externalize_append_char(ctx, '\t') == FALSE)
			return (FALSE);
	}
	if (_prop_object_externalize_append_char(ctx, '<') == FALSE ||
	    _prop_object_externalize_append_cstring(ctx, tag) == FALSE ||
	    _prop_object_externalize_append_char(ctx, '>') == FALSE)
		return (FALSE);
	
	return (TRUE);
}

/*
 * _prop_object_externalize_end_tag --
 *	Append an XML-style end tag to the externalize buffer.
 */
boolean_t
_prop_object_externalize_end_tag(
    struct _prop_object_externalize_context *ctx, const char *tag)
{

	if (_prop_object_externalize_append_char(ctx, '<') == FALSE ||
	    _prop_object_externalize_append_char(ctx, '/') == FALSE ||
	    _prop_object_externalize_append_cstring(ctx, tag) == FALSE ||
	    _prop_object_externalize_append_char(ctx, '>') == FALSE ||
	    _prop_object_externalize_append_char(ctx, '\n') == FALSE)
		return (FALSE);

	return (TRUE);
}

/*
 * _prop_object_externalize_empty_tag --
 *	Append an XML-style empty tag to the externalize buffer.
 */
boolean_t
_prop_object_externalize_empty_tag(
    struct _prop_object_externalize_context *ctx, const char *tag)
{
	unsigned int i;

	for (i = 0; i < ctx->poec_depth; i++) {
		if (_prop_object_externalize_append_char(ctx, '\t') == FALSE)
			return (FALSE);
	}

	if (_prop_object_externalize_append_char(ctx, '<') == FALSE ||
	    _prop_object_externalize_append_cstring(ctx, tag) == FALSE ||
	    _prop_object_externalize_append_char(ctx, '/') == FALSE ||
	    _prop_object_externalize_append_char(ctx, '>') == FALSE ||
	    _prop_object_externalize_append_char(ctx, '\n') == FALSE)
	    	return (FALSE);
	
	return (TRUE);
}

/*
 * _prop_object_externalize_append_cstring --
 *	Append a C string to the externalize buffer.
 */
boolean_t
_prop_object_externalize_append_cstring(
    struct _prop_object_externalize_context *ctx, const char *cp)
{

	while (*cp != '\0') {
		if (_prop_object_externalize_append_char(ctx,
						(unsigned char) *cp) == FALSE)
			return (FALSE);
		cp++;
	}

	return (TRUE);
}

/*
 * _prop_object_externalize_append_encoded_cstring --
 *	Append an encoded C string to the externalize buffer.
 */
boolean_t
_prop_object_externalize_append_encoded_cstring(
    struct _prop_object_externalize_context *ctx, const char *cp)
{

	while (*cp != '\0') {
		switch (*cp) {
		case '<':
			if (_prop_object_externalize_append_cstring(ctx,
					"&lt;") == FALSE)
				return (FALSE);
			break;
		case '>':
			if (_prop_object_externalize_append_cstring(ctx,
					"&gt;") == FALSE)
				return (FALSE);
			break;
		case '&':
			if (_prop_object_externalize_append_cstring(ctx,
					"&amp;") == FALSE)
				return (FALSE);
			break;
		default:
			if (_prop_object_externalize_append_char(ctx,
					(unsigned char) *cp) == FALSE)
				return (FALSE);
			break;
		}
		cp++;
	}

	return (TRUE);
}

#define	BUF_EXPAND		256

/*
 * _prop_object_externalize_append_char --
 *	Append a single character to the externalize buffer.
 */
boolean_t
_prop_object_externalize_append_char(
    struct _prop_object_externalize_context *ctx, unsigned char c)
{

	_PROP_ASSERT(ctx->poec_capacity != 0);
	_PROP_ASSERT(ctx->poec_buf != NULL);
	_PROP_ASSERT(ctx->poec_len <= ctx->poec_capacity);

	if (ctx->poec_len == ctx->poec_capacity) {
		char *cp = _PROP_REALLOC(ctx->poec_buf,
					 ctx->poec_capacity + BUF_EXPAND,
					 M_TEMP);
		if (cp == NULL)
			return (FALSE);
		ctx->poec_capacity = ctx->poec_capacity + BUF_EXPAND;
		ctx->poec_buf = cp;
	}

	ctx->poec_buf[ctx->poec_len++] = c;

	return (TRUE);
}

/*
 * _prop_object_externalize_header --
 *	Append the standard XML header to the externalize buffer.
 */
boolean_t
_prop_object_externalize_header(struct _prop_object_externalize_context *ctx)
{
	static const char _plist_xml_header[] =
"<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
"<!DOCTYPE plist PUBLIC \"-//Apple Computer//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n";

	if (_prop_object_externalize_append_cstring(ctx,
						 _plist_xml_header) == FALSE ||
	    _prop_object_externalize_start_tag(ctx,
				       "plist version=\"1.0\"") == FALSE ||
	    _prop_object_externalize_append_char(ctx, '\n') == FALSE)
		return (FALSE);

	return (TRUE);
}

/*
 * _prop_object_externalize_footer --
 *	Append the standard XML footer to the externalize buffer.  This
 *	also NUL-terminates the buffer.
 */
boolean_t
_prop_object_externalize_footer(struct _prop_object_externalize_context *ctx)
{

	if (_prop_object_externalize_end_tag(ctx, "plist") == FALSE ||
	    _prop_object_externalize_append_char(ctx, '\0') == FALSE)
		return (FALSE);

	return (TRUE);
}

/*
 * _prop_object_externalize_context_alloc --
 *	Allocate an externalize context.
 */
struct _prop_object_externalize_context *
_prop_object_externalize_context_alloc(void)
{
	struct _prop_object_externalize_context *ctx;

	ctx = _PROP_MALLOC(sizeof(*ctx), M_TEMP);
	if (ctx != NULL) {
		ctx->poec_buf = _PROP_MALLOC(BUF_EXPAND, M_TEMP);
		if (ctx->poec_buf == NULL) {
			_PROP_FREE(ctx, M_TEMP);
			return (NULL);
		}
		ctx->poec_len = 0;
		ctx->poec_capacity = BUF_EXPAND;
		ctx->poec_depth = 0;
	}
	return (ctx);
}

/*
 * _prop_object_externalize_context_free --
 *	Free an externalize context.
 */
void
_prop_object_externalize_context_free(
		struct _prop_object_externalize_context *ctx)
{

	/* Buffer is always freed by the caller. */
	_PROP_FREE(ctx, M_TEMP);
}

/*
 * _prop_object_internalize_skip_comment --
 *	Skip the body and end tag of a comment.
 */
static boolean_t
_prop_object_internalize_skip_comment(
				struct _prop_object_internalize_context *ctx)
{
	const char *cp = ctx->poic_cp;

	while (!_PROP_EOF(*cp)) {
		if (cp[0] == '-' &&
		    cp[1] == '-' &&
		    cp[2] == '>') {
			ctx->poic_cp = cp + 3;
			return (TRUE);
		}
		cp++;
	}

	return (FALSE);		/* ran out of buffer */
}

/*
 * _prop_object_internalize_find_tag --
 *	Find the next tag in an XML stream.  Optionally compare the found
 *	tag to an expected tag name.  State of the context is undefined
 *	if this routine returns FALSE.  Upon success, the context points
 *	to the first octet after the tag.
 */
boolean_t
_prop_object_internalize_find_tag(struct _prop_object_internalize_context *ctx,
		      const char *tag, _prop_tag_type_t type)
{
	const char *cp;
	size_t taglen;

	if (tag != NULL)
		taglen = strlen(tag);
	else
		taglen = 0;

 start_over:
	cp = ctx->poic_cp;

	/*
	 * Find the start of the tag.
	 */
	while (_PROP_ISSPACE(*cp))
		cp++;
	if (_PROP_EOF(*cp))
		return (FALSE);

	if (*cp != '<')
		return (FALSE);

	ctx->poic_tag_start = cp++;
	if (_PROP_EOF(*cp))
		return (FALSE);

	if (*cp == '!') {
		if (cp[1] != '-' || cp[2] != '-')
			return (FALSE);
		/*
		 * Comment block -- only allowed if we are allowed to
		 * return a start tag.
		 */
		if (type == _PROP_TAG_TYPE_END)
			return (FALSE);
		ctx->poic_cp = cp + 3;
		if (_prop_object_internalize_skip_comment(ctx) == FALSE)
			return (FALSE);
		goto start_over;
	}

	if (*cp == '/') {
		if (type != _PROP_TAG_TYPE_END &&
		    type != _PROP_TAG_TYPE_EITHER)
			return (FALSE);
		cp++;
		if (_PROP_EOF(*cp))
			return (FALSE);
		ctx->poic_tag_type = _PROP_TAG_TYPE_END;
	} else {
		if (type != _PROP_TAG_TYPE_START &&
		    type != _PROP_TAG_TYPE_EITHER)
			return (FALSE);
		ctx->poic_tag_type = _PROP_TAG_TYPE_START;
	}

	ctx->poic_tagname = cp;

	while (!_PROP_ISSPACE(*cp) && *cp != '/' && *cp != '>')
		cp++;
	if (_PROP_EOF(*cp))
		return (FALSE);

	ctx->poic_tagname_len = cp - ctx->poic_tagname;

	/* Make sure this is the tag we're looking for. */
	if (tag != NULL &&
	    (taglen != ctx->poic_tagname_len ||
	     memcmp(tag, ctx->poic_tagname, taglen) != 0))
		return (FALSE);
	
	/* Check for empty tag. */
	if (*cp == '/') {
		if (ctx->poic_tag_type != _PROP_TAG_TYPE_START)
			return(FALSE);		/* only valid on start tags */
		ctx->poic_is_empty_element = TRUE;
		cp++;
		if (_PROP_EOF(*cp) || *cp != '>')
			return (FALSE);
	} else
		ctx->poic_is_empty_element = FALSE;

	/* Easy case of no arguments. */
	if (*cp == '>') {
		ctx->poic_tagattr = NULL;
		ctx->poic_tagattr_len = 0;
		ctx->poic_tagattrval = NULL;
		ctx->poic_tagattrval_len = 0;
		ctx->poic_cp = cp + 1;
		return (TRUE);
	}

	_PROP_ASSERT(!_PROP_EOF(*cp));
	cp++;
	if (_PROP_EOF(*cp))
		return (FALSE);

	while (_PROP_ISSPACE(*cp))
		cp++;
	if (_PROP_EOF(*cp))
		return (FALSE);

	ctx->poic_tagattr = cp;

	while (!_PROP_ISSPACE(*cp) && *cp != '=')
		cp++;
	if (_PROP_EOF(*cp))
		return (FALSE);

	ctx->poic_tagattr_len = cp - ctx->poic_tagattr;
	
	cp++;
	if (*cp != '\"')
		return (FALSE);
	cp++;
	if (_PROP_EOF(*cp))
		return (FALSE);
	
	ctx->poic_tagattrval = cp;
	while (*cp != '\"')
		cp++;
	if (_PROP_EOF(*cp))
		return (FALSE);
	ctx->poic_tagattrval_len = cp - ctx->poic_tagattrval;
	
	cp++;
	if (*cp != '>')
		return (FALSE);

	ctx->poic_cp = cp + 1;
	return (TRUE);
}

/*
 * _prop_object_internalize_decode_string --
 *	Decode an encoded string.
 */
boolean_t
_prop_object_internalize_decode_string(
				struct _prop_object_internalize_context *ctx,
				char *target, size_t targsize, size_t *sizep,
				const char **cpp)
{
	const char *src;
	size_t tarindex;
	char c;
	
	tarindex = 0;
	src = ctx->poic_cp;

	for (;;) {
		if (_PROP_EOF(*src))
			return (FALSE);
		if (*src == '<') {
			break;
		}

		if ((c = *src) == '&') {
			if (src[1] == 'a' &&
			    src[2] == 'm' &&
			    src[3] == 'p' &&
			    src[4] == ';') {
			    	c = '&';
				src += 5;
			} else if (src[1] == 'l' &&
				   src[2] == 't' &&
				   src[3] == ';') {
				c = '<';
				src += 4;
			} else if (src[1] == 'g' &&
				   src[2] == 't' &&
				   src[3] == ';') {
				c = '>';
				src += 4;
			} else if (src[1] == 'a' &&
				   src[2] == 'p' &&
				   src[3] == 'o' &&
				   src[4] == 's' &&
				   src[5] == ';') {
				c = '\'';
				src += 6;
			} else if (src[1] == 'q' &&
				   src[2] == 'u' &&
				   src[3] == 'o' &&
				   src[4] == 't' &&
				   src[5] == ';') {
				c = '\"';
				src += 6;
			} else
				return (FALSE);
		} else
			src++;
		if (target) {
			if (tarindex >= targsize)
				return (FALSE);
			target[tarindex] = c;
		}
		tarindex++;
	}

	_PROP_ASSERT(*src == '<');
	if (sizep != NULL)
		*sizep = tarindex;
	if (cpp != NULL)
		*cpp = src;
	
	return (TRUE);
}

/*
 * _prop_object_internalize_match --
 *	Returns true if the two character streams match.
 */
boolean_t
_prop_object_internalize_match(const char *str1, size_t len1,
			       const char *str2, size_t len2)
{

	return (len1 == len2 && memcmp(str1, str2, len1) == 0);
}

#define	INTERNALIZER(t, f)			\
{	t,	sizeof(t) - 1,		f	}

static const struct _prop_object_internalizer {
	const char	*poi_tag;
	size_t		poi_taglen;
	prop_object_t	(*poi_intern)(
				struct _prop_object_internalize_context *);
} _prop_object_internalizer_table[] = {
	INTERNALIZER("array", _prop_array_internalize),

	INTERNALIZER("true", _prop_bool_internalize),
	INTERNALIZER("false", _prop_bool_internalize),

	INTERNALIZER("data", _prop_data_internalize),

	INTERNALIZER("dict", _prop_dictionary_internalize),

	INTERNALIZER("integer", _prop_number_internalize),

	INTERNALIZER("string", _prop_string_internalize),

	{ 0, 0, NULL }
};

#undef INTERNALIZER

/*
 * _prop_object_internalize_by_tag --
 *	Determine the object type from the tag in the context and
 *	internalize it.
 */
prop_object_t
_prop_object_internalize_by_tag(struct _prop_object_internalize_context *ctx)
{
	const struct _prop_object_internalizer *poi;

	for (poi = _prop_object_internalizer_table;
	     poi->poi_tag != NULL; poi++) {
		if (_prop_object_internalize_match(ctx->poic_tagname,
						   ctx->poic_tagname_len,
						   poi->poi_tag,
						   poi->poi_taglen))
			return ((*poi->poi_intern)(ctx));
	}

	return (NULL);
}

/*
 * _prop_object_internalize_context_alloc --
 *	Allocate an internalize context.
 */
struct _prop_object_internalize_context *
_prop_object_internalize_context_alloc(const char *xml)
{
	struct _prop_object_internalize_context *ctx;

	ctx = _PROP_MALLOC(sizeof(struct _prop_object_internalize_context),
			   M_TEMP);
	if (ctx == NULL)
		return (NULL);
	
	ctx->poic_xml = ctx->poic_cp = xml;

	/*
	 * Skip any whitespace and XML preamble stuff that we don't
	 * know about / care about.
	 */
	for (;;) {
		while (_PROP_ISSPACE(*xml))
			xml++;
		if (_PROP_EOF(*xml) || *xml != '<')
			goto bad;

#define	MATCH(str)	(memcmp(&xml[1], str, sizeof(str) - 1) == 0)

		/*
		 * Skip over the XML preamble that Apple XML property
		 * lists usually include at the top of the file.
		 */
		if (MATCH("?xml ") ||
		    MATCH("!DOCTYPE plist")) {
			while (*xml != '>' && !_PROP_EOF(*xml))
				xml++;
			if (_PROP_EOF(*xml))
				goto bad;
			xml++;	/* advance past the '>' */
			continue;
		}

		if (MATCH("<!--")) {
			ctx->poic_cp = xml + 4;
			if (_prop_object_internalize_skip_comment(ctx) == FALSE)
				goto bad;
			xml = ctx->poic_cp;
			continue;
		}

#undef MATCH

		/*
		 * We don't think we should skip it, so let's hope we can
		 * parse it.
		 */
		break;
	}

	ctx->poic_cp = xml;
	return (ctx);
 bad:
	_PROP_FREE(ctx, M_TEMP);
	return (NULL);
}

/*
 * _prop_object_internalize_context_free --
 *	Free an internalize context.
 */
void
_prop_object_internalize_context_free(
		struct _prop_object_internalize_context *ctx)
{

	_PROP_FREE(ctx, M_TEMP);
}

#if !defined(_KERNEL) && !defined(_STANDALONE)
/*
 * _prop_object_externalize_file_dirname --
 *	dirname(3), basically.  We have to roll our own because the
 *	system dirname(3) isn't reentrant.
 */
static void
_prop_object_externalize_file_dirname(const char *path, char *result)
{
	const char *lastp;
	size_t len;

	/*
	 * If `path' is a NULL pointer or points to an empty string,
	 * return ".".
	 */
	if (path == NULL || *path == '\0')
		goto singledot;
	
	/* String trailing slashes, if any. */
	lastp = path + strlen(path) - 1;
	while (lastp != path && *lastp == '/')
		lastp--;
	
	/* Terminate path at the last occurrence of '/'. */
	do {
		if (*lastp == '/') {
			/* Strip trailing slashes, if any. */
			while (lastp != path && *lastp == '/')
				lastp--;

			/* ...and copy the result into the result buffer. */
			len = (lastp - path) + 1 /* last char */;
			if (len > (PATH_MAX - 1))
				len = PATH_MAX - 1;

			memcpy(result, path, len);
			result[len] = '\0';
			return;
		}
	} while (--lastp >= path);

 	/* No /'s found, return ".". */
 singledot:
	strcpy(result, ".");
}

/*
 * _prop_object_externalize_write_file --
 *	Write an externalized dictionary to the specified file.
 *	The file is written atomically from the caller's perspective,
 *	and the mode set to 0666 modified by the caller's umask.
 */
boolean_t
_prop_object_externalize_write_file(const char *fname, const char *xml,
    size_t len)
{
	char tname[PATH_MAX];
	int fd;
	int save_errno;

	if (len > SSIZE_MAX) {
		errno = EFBIG;
		return (FALSE);
	}

	/*
	 * Get the directory name where the file is to be written
	 * and create the temporary file.
	 *
	 * We don't use mkstemp() because mkstemp() always creates the
	 * file with mode 0600.  We do, however, use mktemp() safely.
	 */
 again:
	_prop_object_externalize_file_dirname(fname, tname);
	if (strlcat(tname, "/.plistXXXXXX", sizeof(tname)) >= sizeof(tname)) {
		errno = ENAMETOOLONG;
		return (FALSE);
	}
	if (mktemp(tname) == NULL)
		return (FALSE);
	if ((fd = open(tname, O_CREAT|O_RDWR|O_EXCL, 0666)) == -1) {
		if (errno == EEXIST)
			goto again;
		return (FALSE);
	}

	if (write(fd, xml, len) != (ssize_t)len)
		goto bad;

	if (fsync(fd) == -1)
		goto bad;

	(void) close(fd);
	fd = -1;

	if (rename(tname, fname) == -1)
		goto bad;

	return (TRUE);

 bad:
	save_errno = errno;
	if (fd != -1)
		(void) close(fd);
	(void) unlink(tname);
	errno = save_errno;
	return (FALSE);
}

/*
 * _prop_object_internalize_map_file --
 *	Map a file for the purpose of internalizing it.
 */
struct _prop_object_internalize_mapped_file *
_prop_object_internalize_map_file(const char *fname)
{
	struct stat sb;
	struct _prop_object_internalize_mapped_file *mf;
	size_t pgsize = sysconf(_SC_PAGESIZE);
	size_t pgmask = pgsize - 1;
	boolean_t need_guard = FALSE;
	int fd;

	mf = _PROP_MALLOC(sizeof(*mf), M_TEMP);
	if (mf == NULL)
		return (NULL);
	
	fd = open(fname, O_RDONLY, 0400);
	if (fd == -1) {
		_PROP_FREE(mf, M_TEMP);
		return (NULL);
	}

	if (fstat(fd, &sb) == -1) {
		(void) close(fd);
		_PROP_FREE(mf, M_TEMP);
		return (NULL);
	}
	mf->poimf_mapsize = ((size_t)sb.st_size + pgmask) & ~pgmask;
	if (mf->poimf_mapsize < sb.st_size) {
		(void) close(fd);
		_PROP_FREE(mf, M_TEMP);
		return (NULL);
	}

	/*
	 * If the file length is an integral number of pages, then we
	 * need to map a guard page at the end in order to provide the
	 * necessary NUL-termination of the buffer.
	 */
	if ((sb.st_size & pgmask) == 0)
		need_guard = TRUE;

	mf->poimf_xml = mmap(NULL, need_guard ? mf->poimf_mapsize + pgsize
			    		      : mf->poimf_mapsize,
			    PROT_READ, MAP_FILE|MAP_SHARED, fd, (off_t)0);
	(void) close(fd);
	if (mf->poimf_xml == MAP_FAILED) {
		_PROP_FREE(mf, M_TEMP);
		return (NULL);
	}
	(void) madvise(mf->poimf_xml, mf->poimf_mapsize, MADV_SEQUENTIAL);

	if (need_guard) {
		if (mmap(mf->poimf_xml + mf->poimf_mapsize,
			 pgsize, PROT_READ,
			 MAP_ANON|MAP_PRIVATE|MAP_FIXED, -1,
			 (off_t)0) == MAP_FAILED) {
			(void) munmap(mf->poimf_xml, mf->poimf_mapsize);
			_PROP_FREE(mf, M_TEMP);
			return (NULL);
		}
		mf->poimf_mapsize += pgsize;
	}

	return (mf);
}

/*
 * _prop_object_internalize_unmap_file --
 *	Unmap a file previously mapped for internalizing.
 */
void
_prop_object_internalize_unmap_file(
    struct _prop_object_internalize_mapped_file *mf)
{

	(void) madvise(mf->poimf_xml, mf->poimf_mapsize, MADV_DONTNEED);
	(void) munmap(mf->poimf_xml, mf->poimf_mapsize);
	_PROP_FREE(mf, M_TEMP);
}
#endif /* !_KERNEL && !_STANDALONE */

/*
 * Retain / release serialization --
 *
 * Eventually we would like to use atomic operations.  But until we have
 * an MI API for them that is common to userland and the kernel, we will
 * use a lock instead.
 *
 * We use a single global mutex for all serialization.  In the kernel, because
 * we are still under a biglock, this will basically never contend (properties
 * cannot be manipulated at interrupt level).  In userland, this will cost
 * nothing for single-threaded programs.  For multi-threaded programs, there
 * could be contention, but it probably won't cost that much unless the program
 * makes heavy use of property lists.
 */
_PROP_MUTEX_DECL_STATIC(_prop_refcnt_mutex)
#define	_PROP_REFCNT_LOCK()	_PROP_MUTEX_LOCK(_prop_refcnt_mutex)
#define	_PROP_REFCNT_UNLOCK()	_PROP_MUTEX_UNLOCK(_prop_refcnt_mutex)

/*
 * prop_object_retain --
 *	Increment the reference count on an object.
 */
void
prop_object_retain(prop_object_t obj)
{
	struct _prop_object *po = obj;
	uint32_t ocnt;

	_PROP_REFCNT_LOCK();
	ocnt = po->po_refcnt++;
	_PROP_REFCNT_UNLOCK();

	_PROP_ASSERT(ocnt != 0xffffffffU);
}

/*
 * prop_object_release --
 *	Decrement the reference count on an object.
 *
 *	Free the object if we are releasing the final
 *	reference.
 */
void
prop_object_release(prop_object_t obj)
{
	struct _prop_object *po = obj;
	uint32_t ocnt;

	_PROP_REFCNT_LOCK();
	ocnt = po->po_refcnt--;
	_PROP_REFCNT_UNLOCK();

	_PROP_ASSERT(ocnt != 0);
	if (ocnt == 1)
		(*po->po_type->pot_free)(po);
}

/*
 * prop_object_type --
 *	Return the type of an object.
 */
prop_type_t
prop_object_type(prop_object_t obj)
{
	struct _prop_object *po = obj;

	if (obj == NULL)
		return (PROP_TYPE_UNKNOWN);

	return (po->po_type->pot_type);
}

/*
 * prop_object_equals --
 *	Returns TRUE if thw two objects are equivalent.
 */
boolean_t
prop_object_equals(prop_object_t obj1, prop_object_t obj2)
{
	struct _prop_object *po1 = obj1;
	struct _prop_object *po2 = obj2;

	if (po1->po_type != po2->po_type)
		return (FALSE);

	return ((*po1->po_type->pot_equals)(obj1, obj2));
}

/*
 * prop_object_iterator_next --
 *	Return the next item during an iteration.
 */
prop_object_t
prop_object_iterator_next(prop_object_iterator_t pi)
{

	return ((*pi->pi_next_object)(pi));
}

/*
 * prop_object_iterator_reset --
 *	Reset the iterator to the first object so as to restart
 *	iteration.
 */
void
prop_object_iterator_reset(prop_object_iterator_t pi)
{

	(*pi->pi_reset)(pi);
}

/*
 * prop_object_iterator_release --
 *	Release the object iterator.
 */
void
prop_object_iterator_release(prop_object_iterator_t pi)
{

	prop_object_release(pi->pi_obj);
	_PROP_FREE(pi, M_TEMP);
}

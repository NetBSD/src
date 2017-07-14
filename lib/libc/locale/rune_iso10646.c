#include <sys/types.h>
#include <sys/rbtree.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <locale.h>
#include <paths.h>
#include <wchar.h>

#include <iconv.h>
#include <sys/queue.h>
#include "citrus_module.h"
#include "citrus_hash.h"
#include "citrus_iconv.h"

#include "setlocale_local.h"
#include "runetype_local.h"
#include "rune_iso10646.h"
#include "u2k.h"

#ifdef __STDC_ISO_10646__

#define _ISO_10646_CHARSET_NAME "UCS-4"
/* #define DEBUG_ISO10646 */

struct _RuneKutenUnicodeMappingNode {
	wchar_t rkumn_unicode;
	wchar_t rkumn_kuten;
	rb_node_t rkumn_unicode2kuten_node;
	rb_node_t rkumn_kuten2unicode_node;
};

static int _rune_compare_unicode_nodes(void *context, const void *node1, const void *node2);
static int _rune_compare_kuten_nodes(void *context, const void *node1, const void *node2);
static int _rune_compare_unicode_key(void *context, const void *node, const void *key);
static int _rune_compare_kuten_key(void *context, const void *node, const void *key);

static int _rune_compare_unicode_nodes(void *context, const void *node1, const void *node2)
{
	return ((const struct _RuneKutenUnicodeMappingNode *)node2)->rkumn_unicode -
		((const struct _RuneKutenUnicodeMappingNode *)node1)->rkumn_unicode;
}

static int _rune_compare_kuten_nodes(void *context, const void *node1, const void *node2)
{
	return ((const struct _RuneKutenUnicodeMappingNode *)node2)->rkumn_kuten -
		((const struct _RuneKutenUnicodeMappingNode *)node1)->rkumn_kuten;
}

static int _rune_compare_unicode_key(void *context, const void *node, const void *key)
{
	return ((const struct _RuneKutenUnicodeMappingNode *)node)->rkumn_unicode -
		*(const wchar_t *)key;
}

static int _rune_compare_kuten_key(void *context, const void *node, const void *key)
{
	return ((const struct _RuneKutenUnicodeMappingNode *)node)->rkumn_kuten -
		*(const wchar_t *)key;
}

static rb_tree_ops_t kuten2unicode_ops = {
	_rune_compare_kuten_nodes,
	_rune_compare_kuten_key,
	offsetof(struct _RuneKutenUnicodeMappingNode, rkumn_kuten2unicode_node),
	NULL,
};

static rb_tree_ops_t unicode2kuten_ops = {
	_rune_compare_unicode_nodes,
	_rune_compare_unicode_key,
	offsetof(struct _RuneKutenUnicodeMappingNode, rkumn_unicode2kuten_node),
	NULL,
};

void _rune_iso10646_init(struct _RuneLocale *rlp)
{
	rb_tree_init(__UNCONST(rlp->rl_kuten2unicode_rbtree), &kuten2unicode_ops);
	rb_tree_init(__UNCONST(rlp->rl_unicode2kuten_rbtree), &unicode2kuten_ops);
}

/*
 * Convert kuten to unicode and back, and return the result.
 */
#define _RUNE_LOCALE(loc) \
  ((_RuneLocale *)((loc)->part_impl[(size_t)LC_CTYPE]))

int
_citrus_roundtrip(wchar_t , wchar_t *, wchar_t *);

int
_citrus_roundtrip(wchar_t kuten, wchar_t *u, wchar_t *round)
{
	int err0 = 0;

	err0 = _citrus_kuten_to_unicode(_RUNE_LOCALE(_current_locale()), kuten, u);
	if (err0) {
		*u = *round = WEOF;
		errno = err0;
		return -1;
	}
	err0 = _citrus_unicode_to_kuten(_RUNE_LOCALE(_current_locale()), *u, round);
	if (err0) {
		*round = WEOF;
		errno = err0;
		return -1;
	}
	return 0;
}

/*
 * These functions translate between Unicode and the locale's
 * native charset(s) using the structures stores in _RuneLocale.
 *
 * They can fail with ENOMEM, if it is necessary but impossible to add
 * another character pair to the mapping; or the mapping code itself
 * can fail.
 */
int
_citrus_unicode_to_kuten(_RuneLocale *rlp, wchar_t src, wchar_t *dst) {
	struct _citrus_iconv *handle;
	struct _RuneKutenUnicodeMappingNode *node;
	rb_tree_t *rbt = rlp->rl_unicode2kuten_rbtree;
	rb_tree_t *rrbt = rlp->rl_kuten2unicode_rbtree;
	const char *charset = rlp->rl_codeset;
	int ret;

	/* Find the node in the tree, if present */
	node = NULL;
	if (rbt != NULL) {
		node = rb_tree_find_node(rbt, &src);
		if (node) {
			*dst = node->rkumn_kuten;
#ifdef DEBUG_ISO10646
			printf("<uk cached %x -> %x>", src, *dst);
#endif
		}
	}
	if (node == NULL) {
		if (rlp == &_DefaultRuneLocale) {
			*dst = src;
#ifdef DEBUG_ISO10646
			printf("<uk default %x -> %x>", src, *dst);
#endif
			return 0;
		}

		/* Shortcut for ISO8859-1 */
		if (strcmp(charset, "646") == 0) { /* XXX bad test */
			*dst = src;
#ifdef DEBUG_ISO10646
			printf("<uk iso8859-1 %x -> %x>", src, *dst);
#endif
			return 0;
		}

		/* Fix up charset if we are using an encoding that
		   uses multiple charsets (e.g. ISO2022) */
		if (strncmp(charset, "ISO2022", 7) == 0) {
			struct unicode2kuten_lookup *ks;
			
			/* Our encoding of ISO2022 is compatible with Unicode for small code points */
			if (src < 0x100) {
				*dst = src;
				return 0;
			}
			
#ifdef DEBUG_ISO10646
			printf("<iso2022 u2k k=%x", src);
#endif
			ks = unicode2kuten(src);
			if (ks == NULL) {
#ifdef DEBUG_ISO10646
				printf(" FAILED>");
#endif
				*dst = WEOF;
				return EILSEQ;
			}
			*dst = ks->value;
#ifdef DEBUG_ISO10646
			printf(" -> %x>", *dst);
#endif
			return 0;
		}
		
		/* XXX this is probably very inefficient */
		ret = _citrus_iconv_open(&handle, _PATH_ICONV, _ISO_10646_CHARSET_NAME, charset);
		if (ret) {
#ifdef DEBUG_ISO10646
			fprintf(stderr, "<iconv_open UCS4->%s FAILED with errno=%d>\n", charset, ret);
#endif
			return ret;
		}
		ret = _citrus_iconv_wchar_convert(handle, src, dst);
		if (ret) {
#ifdef DEBUG_ISO10646
			fprintf(stderr, "<wchar_convert UCS4->%s FAILED with errno=%d>\n", charset, ret);
#endif
			return ret;
		}
		_citrus_iconv_close(handle);
		
		/*
		 * Store new node in the tree; but
		 * don't store WEOF.  If we're reading
		 * garbage, it's preferably to be slow
		 * than to crash.
		 */
		if (rbt && *dst != WEOF) {
			node = malloc(sizeof(*node));
			if (node != NULL) {
				node->rkumn_unicode = src;
				node->rkumn_kuten = *dst;
				rb_tree_insert_node(rbt, node);
				rb_tree_insert_node(rrbt, node);
			}
#ifdef DEBUG_ISO10646
			printf("<uk %x -> %x>", src, *dst);
#endif
		}
	}

	return 0;
}

#ifdef USE_ISO2022_ICONV

#define ISO2022_CHARSET_MASK 0x7F808080

static const char *iso2022_cs2charset(wchar_t, wchar_t *);

static const char *iso2022_cs2charset(wchar_t src, wchar_t *kutenp)
{
	*kutenp &= ~ISO2022_CHARSET_MASK;
	switch (src & ISO2022_CHARSET_MASK) {
	case 0x00000000: /* (B: US-ASCII */
	case 0x00000080: /* ISO8859-1 */
		return "US-ASCII";

#if 0
		/* Template, taken from comment in citrus_iso2022.c */
	case 0xXX000000: /* Template ESC $ ( X, 2- or 3-byte */
	case 0xXX000000: /* Template ESC ( X, 1-byte */
	case 0xXX000080: /* Template ESC $ , X, 2- or 3-byte */
	case 0xXX000080: /* Template ESC , X, 1-byte */
	case 0xXXMm0000: /* Template ESC & M ESC $ ( X, 2-byte */
	case 0xXXMm0000: /* Template ESC ( M X, 1 byte */
	case 0xXXMm0080: /* Template ESC , M X, 1 byte */
#endif

		/* ISO-2022-JP */
	case 0x4A000080: /* (J: JIS X 0201 left (Roman) */
		return "US-ASCII"; /* XXX Yen sign */
	case 0x40000000: /* $(@: JIS X 0208-1978 */
		return "JIS_X0208";
	case 0x42000000: /* $(B: JIS X 0208-1983 */
		return "JIS_X0208-1983";

		/* ISO-2022-JP-1 */
	case 0x44000000: /* $(D: JIS X 0212-1990 */
		return "JIS_X0212";

		/* ISO-2022-JP-2 */
	case 0x41000000: /* $(A: GB 2312-1980 ; also in ISO-2022-CN */
		return "GB2312";
	case 0x43000000: /* $(C: KSC 5601-1987 / KS X 1001-1992 ; also in ISO-2022-KR */
		return "KSC_5601";
	case 0x41000080: /* ,A: ISO8859-1 */
		return "ISO-8859-1";
	case 0x46000080: /* ,F: ISO8859-7 */
		return "ISO-8859-7";

		/* ISO-2022-JP-3 */
	case 0x49000080: /* (I: JIS X 0201 right (Kana) */
		return "JIS_X0201";
	case 0x4F000000: /* $(O: JIS X 0213-2000 Plane 1 */
	case 0x50000000: /* $(P: JIS X 0213-2000 Plane 2 */
		*kutenp |= (src & 0xFF000000) - 0x4E000000;
		return "SHIFT_JISX0213";

		/* ISO-2022-JP-2004 */
	case 0x51000000: /* $(Q: JIS X 0213-2004 Plane 1 */
		return "SHIFT_JISX0213";

		/* ISO-2022-CN */
	case 0x47000000: /* $(G: CNS 11643-1992 Plane 1 */
	case 0x48000000: /* $(H: CNS 11643-1992 Plane 2 */
		*kutenp |= (src & 0x0F000000) - 0x06000000; /* Plane designator */
		return "CNS11643";

		/* ISO-2022-CN-EXT */
	case 0x45000000: /* $(E: ISO-IR-165 */
		return "ISO-IR-165";
	case 0x49000000: /* $(I: CNS 11643-1992 Plane 3 */
	case 0x4A000000: /* $(J: CNS 11643-1992 Plane 4 */
	case 0x4B000000: /* $(K: CNS 11643-1992 Plane 5 */
	case 0x4C000000: /* $(L: CNS 11643-1992 Plane 6 */
	case 0x4D000000: /* $(M: CNS 11643-1992 Plane 7 */
		*kutenp |= (src & 0x0F000000) - 0x06000000; /* Plane designator */
		return "CNS11643";

		/* Misc, taken from CHARSET values in share/locale/ctype/charset/ */
	case 0x42000080: /* ,B: ISO8859-2 */
		return "ISO-8859-2";
	case 0x43000080: /* ,C: ISO8859-3 */
		return "ISO-8859-3";
	case 0x44000080: /* ,D: ISO8859-4 */
		return "ISO-8859-4";
	case 0x48000080: /* ,H: ISO8859-8 */
		return "ISO-8859-8";
	case 0x4C000080: /* ,L: ISO8859-5 */
		return "ISO-8859-5";
	case 0x4D000080: /* ,M: ISO8859-9 */
		return "ISO-8859-9";
	case 0x56000080: /* ,V: ISO8859-10 */
		return "ISO-8859-10";
	case 0x58000080: /* ,X: "Latin-6+" ISO8859-10 again? XXX */
		return "ISO-8859-10";

	default:
		break;
	}

	return "UNKNOWN";
}
#endif

int
_citrus_kuten_to_unicode(_RuneLocale *rlp, wchar_t src, wchar_t *dst) {
	struct _citrus_iconv *handle;
	struct _RuneKutenUnicodeMappingNode *node;
	rb_tree_t *rbt = rlp->rl_kuten2unicode_rbtree;
	rb_tree_t *rrbt = rlp->rl_unicode2kuten_rbtree;
	/* The iconv name of the char set, often the given codeset */
	const char *charset = rlp->rl_codeset;
	wchar_t kuten;
	int ret;

	/* Shortcut for C locale */
	if (rlp == &_DefaultRuneLocale) {
		*dst = src;
#ifdef DEBUG_ISO10646
		printf("<uk default %x -> %x>", src, *dst);
#endif
		return 0;
	}

	/* Shortcut for ISO8859-1 */
	if (strcmp(charset, "646") == 0) { /* XXX bad test */
		*dst = src;
#ifdef DEBUG_ISO10646
		printf("<uk iso8859-1 %x -> %x>", src, *dst);
#endif
		return 0;
	}
	
	/* Fix up charset if we are using an encoding that
	   uses multiple charsets (e.g. ISO2022) */
	kuten = src;
	if (strncmp(charset, "ISO2022", 7) == 0) {
		/* Our encoding of ISO2022 is compatible
		   with Unicode for small code points */
		if (src < 0x100) {
			*dst = src;
			return 0;
		} else {
#ifdef USE_ISO2022_ICONV
			charset = iso2022_cs2charset(src, &kuten);
#else
			struct unicode2kuten_lookup *ks;
		
#ifdef DEBUG_ISO10646
			printf("<iso2022 k2u k=%x", src);
#endif
			ks = kuten2unicode(src);
			if (ks == NULL) {
#ifdef DEBUG_ISO10646
				printf(" not found>");
#endif
				*dst = WEOF;
				return EILSEQ;
			}
			*dst = ks->value;
#ifdef DEBUG_ISO10646
			printf("-> %x>", *dst);
#endif
			return 0;
#endif
		}
	}

	/* Find the node in the tree, if present */
	node = NULL;
	if (rbt != NULL) {
		node = rb_tree_find_node(rbt, &src);
		if (node) {
			*dst = node->rkumn_unicode;
#ifdef DEBUG_ISO10646
			printf("<ku cached %x -> %x>", src, *dst);
#endif
		}
	}
	if (node == NULL) {
		/* XXX this is probably very inefficient */
		ret = _citrus_iconv_open(&handle, _PATH_ICONV, charset, _ISO_10646_CHARSET_NAME);
		if (ret) {
			*dst = WEOF;
			return ret;
		}
		/* Translate the kuten value rather than the raw source value */
		ret = _citrus_iconv_wchar_convert(handle, kuten, dst);
		if (ret) {
			*dst = WEOF;
			return ret;
		}
		_citrus_iconv_close(handle);
		
		/*
		 * Store new node in the tree; but
		 * don't store WEOF.  If we're reading
		 * garbage, it's preferable to be slow
		 * than to crash.
		 */
		if (rbt && *dst != WEOF) {
			node = malloc(sizeof(*node));
			if (node != NULL) {
				node->rkumn_kuten = src;
				node->rkumn_unicode = *dst;
				rb_tree_insert_node(rbt, node);
				rb_tree_insert_node(rrbt, node);
			}
#ifdef DEBUG_ISO10646
			printf("<ku %x -> %x>", src, *dst);
#endif
		}
	}

	return 0;
}

int
_citrus_wcs_unicode_to_kuten(_RuneLocale *rlp, const wchar_t *src, wchar_t *dst, size_t n) {
	const wchar_t *end = src + n;
	const wchar_t *sp;
	wchar_t *dp;
	int error;

	for (sp = src, dp = dst; sp < end; ++sp, ++dp) {
		if (*sp == 0) {
			*dp = 0;
			return 0;
		}
		error = _citrus_unicode_to_kuten(rlp, *sp, dp);
		if (error)
			return error;
	}
	return 0;
}

int
_citrus_wcs_kuten_to_unicode(_RuneLocale *rlp, const wchar_t *src, wchar_t *dst, size_t n) {
	const wchar_t *end = src + n;
	const wchar_t *sp;
	wchar_t *dp;
	int error;

	for (sp = src, dp = dst; sp < end; ++sp, ++dp) {
		if (*sp == 0) {
			*dp = 0;
			return 0;
		}
		error = _citrus_kuten_to_unicode(rlp, *sp, dp);
		if (error)
			return error;
	}
	return 0;
}
#endif /* __STDC_ISO_10646__ */

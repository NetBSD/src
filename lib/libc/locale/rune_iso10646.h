#ifdef __STDC_ISO_10646__

__BEGIN_DECLS
void _rune_iso10646_init(struct _RuneLocale *);
int _citrus_unicode_to_kuten(_RuneLocale *, wchar_t, wchar_t *);
int _citrus_kuten_to_unicode(_RuneLocale *, wchar_t, wchar_t *);
int _citrus_wcs_unicode_to_kuten(_RuneLocale *, const wchar_t *,
				 wchar_t *, size_t);
int _citrus_wcs_kuten_to_unicode(_RuneLocale *, const wchar_t *,
				 wchar_t *, size_t);
__END_DECLS

#else /* ! __STDC_ISO_10646__ */

#include <assert.h>

#define _rune_iso10646_init(x)

static __inline int
_citrus_unicode_to_kuten(_RuneLocale *rlp, wchar_t src, wchar_t *dst) {
	*dst = src;
	return 0;
}

static __inline int
_citrus_kuten_to_unicode(_RuneLocale *rlp, wchar_t src, wchar_t *dst) {
	*dst = src;
	return 0;
}

/* Be careful with these - usually we need to #ifdef around them instead */
static __inline int
_citrus_wcs_unicode_to_kuten(_RuneLocale *rlp, const wchar_t *src, wchar_t *dst, size_t n) {
	assert(dst == src);
	return 0;
}

static __inline int
_citrus_wcs_kuten_to_unicode(_RuneLocale *rlp, const wchar_t *src, wchar_t *dst, size_t n) {
	assert(dst == src);
	return 0;
}

#endif /*! __STDC_ISO_10646__ */

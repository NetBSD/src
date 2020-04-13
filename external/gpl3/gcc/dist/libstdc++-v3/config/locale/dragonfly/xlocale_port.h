#ifndef __NetBSD__
#include <xlocale.h>

static __inline int
MB_CUR_MAX_L(locale_t __l) {
	locale_t __old = uselocale(__l);
	size_t __rv = MB_CUR_MAX;
	(void)uselocale(__old);
	return __rv;
}

static __inline size_t
wcsnrtombs_l(char * __restrict __to, const wchar_t ** __restrict __from,
    size_t __nwc, size_t __len, mbstate_t * __restrict __st, locale_t __l)
{
	locale_t __old = uselocale(__l);
	size_t __rv = wcsnrtombs(__to, __from, __nwc, __len, __st);
	(void)uselocale(__old);
	return __rv;
}

static __inline size_t
mbsnrtowcs_l(wchar_t * __restrict __to, const char ** __restrict __from,
    size_t __nms, size_t __len, mbstate_t * __restrict __st, locale_t __l)
{
	locale_t __old = uselocale(__l);
	size_t __rv = mbsnrtowcs(__to, __from, __nms, __len, __st);
	(void)uselocale(__old);
	return __rv;
}

static __inline size_t
wcrtomb_l(char * __restrict __to, wchar_t __from,
    mbstate_t * __restrict __st, locale_t __l)
{
	locale_t __old = uselocale(__l);
	size_t __rv = wcrtomb(__to, __from, __st);
	(void)uselocale(__old);
	return __rv;
}

static __inline size_t
mbrtowc_l(wchar_t * __restrict __to, const char * __restrict __from,
    size_t __len, mbstate_t * __restrict __st, locale_t __l)
{
	locale_t __old = uselocale(__l);
	size_t __rv = mbrtowc(__to, __from, __len, __st);
	(void)uselocale(__old);
	return __rv;
}

static __inline size_t
mbsrtowcs_l(wchar_t * __restrict __to, const char ** __restrict __from,
    size_t __len, mbstate_t * __restrict __st, locale_t __l)
{
	locale_t __old = uselocale(__l);
	size_t __rv = mbsrtowcs(__to, __from, __len, __st);
	(void)uselocale(__old);
	return __rv;
}

static __inline int
wctob_l(wint_t __wc, locale_t __l)
{
	locale_t __old = uselocale(__l);
	int __rv = wctob(__wc);
	(void)uselocale(__old);
	return __rv;
}
#endif

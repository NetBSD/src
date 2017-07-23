static __inline int
/*ARGSUSED*/
_FUNCNAME(ucs2kt)(_ENCODING_INFO * __restrict ei,
		  wchar_kuten_t * __restrict ktp, wchar_ucs4_t wc)
{

	_DIAGASSERT(ktp != NULL);

	*ktp = wc;

	return 0;
}

static __inline int
/*ARGSUSED*/
_FUNCNAME(kt2ucs)(_ENCODING_INFO * __restrict ei,
		  wchar_ucs4_t * __restrict up, wchar_kuten_t kt)
{

	_DIAGASSERT(up != NULL);
	
	*up = kt;

	return 0;
}

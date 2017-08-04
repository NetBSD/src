#if HAVE_NBTOOL_CONFIG_H
#include "nbtool_config.h"
#endif

#include "../../lib/libc/citrus/modules/citrus_iso2022.c"

int   
citrus_ISO2022_mbtowc(wchar_t *, const char *, unsigned, const char *);
int
citrus_ISO2022_wcstombs(char *, wchar_t, const char *);

static _ISO2022EncodingInfo ei;
static _ISO2022State state;
static int inited;

int
citrus_ISO2022_mbtowc(wchar_t *wc, const char *mb, unsigned n, const char *variable)
{
	size_t tnb = 0;

 	if (!inited) {
	  _citrus_ISO2022_init_state(&ei, &state);
	  _citrus_ISO2022_encoding_module_init(&ei, (void *)variable, strlen(variable));
	  inited = 1;
	}
	_citrus_ISO2022_mbrtowc_priv(&ei, wc, &mb, n, &state, &tnb);

	return tnb;
}

int
citrus_ISO2022_wcstombs(char *mb, wchar_t wc, const char *variable)
{
	size_t tnb = 0;

 	if (!inited) {
	  _citrus_ISO2022_init_state(&ei, &state);
	  _citrus_ISO2022_encoding_module_init(&ei, (void *)variable, strlen(variable));
	}
	_citrus_ISO2022_wcrtomb_priv(&ei, mb, (size_t)-1, wc, &state, &tnb);

	return tnb;
}

#ifdef MAIN
#include <vis.h>

int main(int argc, char **argv)
{
	char    test_mbs[] = "\033$BF|K\1348l\033(BA\033$B$\"\033(BB\033$B$$\033(B";
	wchar_t test_wcs[] = { 0x4200467c, 0x42004b5c, 0x4200386c, 0x41, 0x42002422, 0x42, 0x42002424, 0 };
	int i, j;
	size_t r;
	char mbs_result[1024];
	wchar_t wcs_result[1024], wc;
	char vis[1024];

	j = 0;
	for (i = 0; test_mbs[i] != 0 && r > 0; i += r) {
	  r = citrus_ISO2022_mbtowc(wcs_result, test_mbs + i, UINT_MAX, "INIT0=94B CODESET=ISO2022-JP");
		strvisx(vis, test_mbs + i, r, VIS_OCTAL);
		fprintf(stderr, "mbtowc: %d converted %s to %x", r, vis, *wcs_result);
		if (*wcs_result != test_wcs[j])
			fprintf(stderr, " (expected %x)", test_wcs[j]);
		fprintf(stderr, "\n");
		++j;
	}

	j = 0;
	memset(mbs_result, 0, sizeof(mbs_result));
	for (i = 0; i == 0 || test_wcs[i - 1] != 0; i++) {
		wc = test_wcs[i];
		r = citrus_ISO2022_wcstombs(mbs_result + j, wc, "INIT0=94B CODESET=ISO2022-JP");
		mbs_result[j + r] = '\0';
		strvisx(vis, mbs_result + j, r, VIS_OCTAL);
		fprintf(stderr, "wsctombs: %d converted %x to %s\n", r, test_wcs[i], vis);
		j += r;
	}

	// Feed it back through and see if it comes out the same
	memset(&ei, 0, sizeof(ei));
	memset(&state, 0, sizeof(state));
	_citrus_ISO2022_init_state(&ei, &state);
	_citrus_ISO2022_encoding_module_init(&ei, (void *)mbs_result, UINT_MAX);
	j = 0;
	for (i = 0; mbs_result[i] != 0 && r > 0; i += r) {
		r = citrus_ISO2022_mbtowc(wcs_result, mbs_result + i, UINT_MAX, "INIT0=94B CODESET=ISO2022-JP");
		strvisx(vis, mbs_result + i, r, VIS_OCTAL);
		fprintf(stderr, "mbtowc: %d converted %s to %x", r, vis, *wcs_result);
		if (*wcs_result != test_wcs[j])
			fprintf(stderr, " (expected %x)", test_wcs[j]);
		fprintf(stderr, "\n");
		++j;
	}
}
#endif


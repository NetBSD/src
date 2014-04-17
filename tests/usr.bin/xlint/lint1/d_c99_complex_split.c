void a(void) {
    double _Complex z = 0;
    if (__builtin_isnan((__real__ z)) && __builtin_isnan((__imag__ z)))
	return;
}

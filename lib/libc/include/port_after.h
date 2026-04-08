
/*
 * dst: output buffer
 * l: local length
 * t: index into dst
 * size: size of output buffer
 * emsgsize: label to go on error.
 */

/*
 * Check for space to add a character and NUL terminate
 */
#define ADDC(C) \
	do { \
		if ((size_t)(t + 2) >= size) \
			goto emsgsize; \
		dst[t++] = (C); \
		dst[t] = '\0'; \
	} while (0)

/*
 * Call S that appends to the buffer, check that it fit, and move
 * the index.
 */
#define ADDS(S) \
	do { \
		l = (S); \
		if (l < 0 || (size_t)(l + t) >= size) \
			goto emsgsize; \
		t += l; \
	} while (0)

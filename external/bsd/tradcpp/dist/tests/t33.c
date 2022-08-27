/* make sure that R gets defined and doesn't end up part of a string */
#define Q "
#define R r
#define S "
R
Q

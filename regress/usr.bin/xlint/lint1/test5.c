/* pointer casts are valid lhs lvalues */

void
foo() {
    unsigned long p = 6;
    ((struct sockaddr *)p) = 0;
}

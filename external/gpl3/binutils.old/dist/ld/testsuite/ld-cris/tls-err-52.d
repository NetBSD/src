#source: tls-le-12s.s
#as: --no-underscore --em=criself
#ld: -m crislinux --shared
#error: \A[^\n]*\.o, [^\n]*\n[^\n]*mixup[^\n]*\n[^\n]*Invalid operation\Z

# Undefined reference for a R_CRIS_16_TPREL in a DSO (where it's
# invalid in the first place anyway, so we should see the same
# behavior as for that test, tls-err-24.d).

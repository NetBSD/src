#source: tls-ld-4.s
#source: tls-x.s
#as: --no-underscore --em=criself --pic
#ld: --shared -m crislinux
#error: \A[^\n]*\.o,[^\n]*mixup[^\n]*\n[^\n]*Bad value\Z

# Check that R_CRIS_16_DTPREL on non-module-local symbol in input to a
# DSO is flagged as an error.

#! /usr/bin/sed -f
#
# each line of the form ^.. .* contains the code for a country.
#
/^.. / {
  h
  s/^.. \(.*\)/\1./
  x
  s/^\(..\).*/@item \1/
  G
  p
}
#
# delete the rest
#
d

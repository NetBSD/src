#! /usr/bin/sed -f
#
# each line of the form ^..<tab>.* contains the code for a country.
#
/^..	/ {
  h
  s/^..	\(.*\)/\1./
  s/\&/and/g
  s/ô/@^{o}/g
  s/é/e/g
  x
  s/^\(..\).*/@item \1/
  G
  p
}
#
# delete the rest
#
d

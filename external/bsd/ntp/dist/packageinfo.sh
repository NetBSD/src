repotype=dev
# version=Protocol.Major.Minor
# odd minor numbers are for -dev, even minor numbers are for -stable
proto=4
major=2
minor=6
version=${proto}.${major}.${minor}
# Point.  3 cases:
# - Numeric values increment
# - empty 'increments' to 1
# - NEW 'increments' to empty
point=
# Special.  Normally unused.  A suffix.
#special=ag
special=
# ReleaseCandidate. 'yes' or 'no'.
#releasecandidate=yes
releasecandidate=no
# ChangeLog tag
CLTAG=NTP_4_2_0
###
# The following is for ntp-stable.  2 cases:
# - Numeric values increment
# - GO triggers a release
# - - rcpoint gets set to 0
# - - releasecandidate gets set to no
# - GRONK is for -dev
rcpoint=1

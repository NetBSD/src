#!/usr/bin/env perl
#
# Copyright (C) Internet Systems Consortium, Inc. ("ISC")
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# See the COPYRIGHT file distributed with this work for additional
# information regarding copyright ownership.

use strict;
use warnings;

sub process_changeset;

my @changeset;

while( my $line = <> ) {
    chomp $line;

    if( $line =~ /^(?<op>add|del) (?<label>\S+)\s+(?<ttl>\d+)\s+IN\s+(?<rrtype>\S+)\s+(?<rdata>.*)/ ) {
        my $change = {
            op     => $+{op},
            label  => $+{label},
            ttl    => $+{ttl},
            rrtype => $+{rrtype},
            rdata  => $+{rdata},
        };

        if( $change->{op} eq 'del' and $change->{rrtype} eq 'SOA' ) {
            if( @changeset ) {
                process_changeset( @changeset );
                @changeset = ();
            }
        }

        push @changeset, $change;
    }
    else {
        die "error parsing journal data";
    }
}

if( @changeset ) {
    process_changeset( @changeset );
}

{
    my %rrsig_db;
    my %keys;
    my $apex;

    sub process_changeset {
        my @changeset = @_;

        if( not $apex ) {
            # the first record of the first changeset is guaranteed to be the apex
            $apex = $changeset[0]{label};
        }

        my $newserial;
        my %touched_rrsigs;
        my %touched_keys;

        foreach my $change( @changeset ) {
            if( $change->{rrtype} eq 'SOA' ) {
                if( $change->{op} eq 'add' ) {
                    if( $change->{rdata} !~ /^\S+ \S+ (?<serial>\d+)/ ) {
                        die "unable to parse SOA";
                    }

                    $newserial = $+{serial};
                }
            }
            elsif( $change->{rrtype} eq 'NSEC' ) {
                ; # do nothing
            }
            elsif( $change->{rrtype} eq 'DNSKEY' ) {
                ; # ignore for now
            }
            elsif( $change->{rrtype} eq 'TYPE65534' and $change->{label} eq $apex ) {
                # key status
                if( $change->{rdata} !~ /^\\# (?<datasize>\d+) (?<data>[0-9A-F]+)$/ ) {
                    die "unable to parse key status record";
                }

                my $datasize = $+{datasize};
                my $data = $+{data};

                if( $datasize == 5 ) {
                    my( $alg, $id, $flag_del, $flag_done ) = unpack 'CnCC', pack( 'H10', $data );

                    if( $change->{op} eq 'add' ) {
                        if( not exists $keys{$id} ) {
                            $touched_keys{$id} //= 1;

                            $keys{$id} = {
                                $data        => 1,
                                rrs          => 1,
                                done_signing => $flag_done,
                                deleting     => $flag_del,
                            };
                        }
                        else {
                            if( not exists $keys{$id}{$data} ) {
                                my $keydata = $keys{$id};
                                $touched_keys{$id} = { %$keydata };

                                $keydata->{rrs}++;
                                $keydata->{$data} = 1;
                                $keydata->{done_signing} += $flag_done;
                                $keydata->{deleting} += $flag_del;
                            }
                        }
                    }
                    else {
                        # this logic relies upon the convention that there won't
                        # ever be multiple records with the same flag set
                        if( exists $keys{$id} ) {
                            my $keydata = $keys{$id};

                            if( exists $keydata->{$data} ) {
                                $touched_keys{$id} = { %$keydata };

                                $keydata->{rrs}--;
                                delete $keydata->{$data};
                                $keydata->{done_signing} -= $flag_done;
                                $keydata->{deleting} -= $flag_del;

                                if( $keydata->{rrs} == 0 ) {
                                    delete $keys{$id};
                                }
                            }
                        }
                    }
                }
                else {
                    die "unexpected key status record content";
                }
            }
            elsif( $change->{rrtype} eq 'RRSIG' ) {
                if( $change->{rdata} !~ /^(?<covers>\S+) \d+ \d+ \d+ (?<validity_end>\d+) (?<validity_start>\d+) (?<signing_key>\d+)/ ) {
                    die "unable to parse RRSIG rdata";
                }

                $change->{covers} = $+{covers};
                $change->{validity_end} = $+{validity_end};
                $change->{validity_start} = $+{validity_start};
                $change->{signing_key} = $+{signing_key};

                my $db_key = $change->{label} . ':' . $change->{covers};

                $rrsig_db{$db_key} //= {};
                $touched_rrsigs{$db_key} = 1;

                if( $change->{op} eq 'add' ) {
                    $rrsig_db{$db_key}{ $change->{signing_key} } = 1;
                }
                else {
                    # del
                    delete $rrsig_db{$db_key}{ $change->{signing_key} };
                }
            }
        }

        foreach my $key_id( sort keys %touched_keys ) {
            my $old_data;
            my $new_data;

            if( ref $touched_keys{$key_id} ) {
                $old_data = $touched_keys{$key_id};
            }

            if( exists $keys{$key_id} ) {
                $new_data = $keys{$key_id};
            }

            if( $old_data ) {
                if( $new_data ) {
                    print "at serial $newserial key $key_id status changed from ($old_data->{deleting},$old_data->{done_signing}) to ($new_data->{deleting},$new_data->{done_signing})\n";
                }
                else {
                    print "at serial $newserial key $key_id status removed from zone\n";
                }
            }
            else {
                print "at serial $newserial key $key_id status added with flags ($new_data->{deleting},$new_data->{done_signing})\n";
            }
        }

        foreach my $rrsig_id( sort keys %touched_rrsigs ) {
            my $n_signing_keys = keys %{ $rrsig_db{$rrsig_id} };

            if( $n_signing_keys == 0 ) {
                print "at serial $newserial $rrsig_id went unsigned\n";
            }
            elsif( $rrsig_id =~ /:DNSKEY$/ ) {
                if( $n_signing_keys != 2 ) {
                    print "at serial $newserial $rrsig_id was signed $n_signing_keys time(s) when it should have been signed twice\n";
                }
            }
            elsif( $n_signing_keys > 1 ) {
                my @signing_keys = sort { $a <=> $b } keys %{ $rrsig_db{$rrsig_id} };
                print "at serial $newserial $rrsig_id was signed too many times, keys (@signing_keys)\n";
            }
        }
    }
}

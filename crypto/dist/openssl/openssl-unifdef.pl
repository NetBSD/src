#! /usr/pkg/bin/perl
$tmp=tmpfile.$$;

$unifdef0 = <<EOF;
NO_ASN1_TYPEDEFS
NO_BF
NO_BIO
NO_BUFFER
NO_CAST
NO_COMP
NO_DES
NO_DH
NO_DSA
NO_ERR
NO_EVP
NO_FP_API
NO_HASH_COMP
NO_HMAC
NO_IDEA
NO_LHASH
NO_LOCKING
NO_MD2
NO_MD4
NO_MD5
NO_MDC2
NO_RC2
NO_RC4
NO_RC5
NO_RIPEMD
NO_RSA
NO_SHA
NO_SHA0
NO_SHA1
NO_SOCK
NO_SSL2
NO_SSL3
NO_STACK
NO_STDIO
NO_X509
PEDANTIC
EOF

$unifdef = $unifdef0;
$unifdef =~ s/\n$//;
$unifdef =~ s/^/-U/;
$unifdef =~ s/\n/\n-U/g;
$unifdef =~ s/\n/ /g;
$unifdef =~ join("\n", $unifdef);

$files = `find . -name \\\*.h -print | grep -v MacOS`;
foreach $i (split(/[\n ]/, $files)) {
	print "unifdef $unifdef <$i >$tmp\n";
	print "mv $tmp $i\n";
}

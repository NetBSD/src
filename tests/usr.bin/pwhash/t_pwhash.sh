atf_test_case pwhash_blowfish_r12
pwhash_blowfish_r12_head() {
	atf_set "descr" "ATF test for pwhash using blowfish 12 rounds"
}

pwhash_blowfish_r12_body() {
	atf_check -s exit:0 -o match:"^\\\$2a\\\$" -x \
		'echo -n password | pwhash -b 12'
}

atf_test_case pwhash_md5
pwhash_md5_head() {
	atf_set "descr" "ATF test for pwhash using MD5"
}

pwhash_md5_body() {
	atf_check -s exit:0 -o match:"^\\\$1\\\$" -x \
		'echo -n password | pwhash -m'
}

atf_test_case pwhash_sha1
pwhash_sha1_head() {
	atf_set "descr" "ATF test for pwhash using SHA1"
}

pwhash_sha1_body() {
	atf_check -s exit:0 -o match:"^\\\$sha1\\\$" -x \
		'echo -n password | pwhash'
}

atf_test_case pwhash_des
pwhash_des_head() {
	atf_set "descr" "ATF test for pwhash using DES"
}

pwhash_des_body() {
	atf_check -s exit:0 -o ignore -e ignore -x \
		'echo -n password | pwhash -s somesalt'
}

atf_init_test_cases()
{
	atf_add_test_case pwhash_blowfish_r12
	atf_add_test_case pwhash_md5
	atf_add_test_case pwhash_sha1
	atf_add_test_case pwhash_des
}

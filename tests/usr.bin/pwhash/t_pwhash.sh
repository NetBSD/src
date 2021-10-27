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
		'echo -n password | pwhash -S 24680'
}

atf_test_case pwhash_argon2i
pwhash_argon2i_head() {
	atf_set "descr" "ATF test for pwhash using Argon2i"
}

pwhash_argon2i_body() {
	atf_check -s exit:0 \
		-o match:"^\\\$argon2i\\\$v=19\\\$m=1024,t=1,p=1\\\$" -x \
		'echo -n password | pwhash -A argon2i,m=1024,t=1'
}

atf_test_case pwhash_argon2id
pwhash_argon2id_head() {
	atf_set "descr" "ATF test for pwhash using Argon2id"
}

pwhash_argon2id_body() {
	atf_check -s exit:0 \
		-o match:"^\\\$argon2id\\\$v=19\\\$m=256,t=3,p=1\\\$" -x \
		'echo -n password | pwhash -A argon2id,m=256,t=3'
}

atf_test_case pwhash_argon2d
pwhash_argon2d_head() {
	atf_set "descr" "ATF test for pwhash using Argon2d"
}

pwhash_argon2d_body() {
	atf_check -s exit:0 \
		-o match:"^\\\$argon2d\\\$v=19\\\$" -x \
		'echo -n password | pwhash -A argon2d'
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
	atf_add_test_case pwhash_argon2i
	atf_add_test_case pwhash_argon2id
	atf_add_test_case pwhash_argon2d
	atf_add_test_case pwhash_des
}

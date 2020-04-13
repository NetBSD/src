atf_test_case argon2_argon2id 
argon2_argon2id_head() {
	atf_set "descr" "ATF test for argon2 argon2id variant"
}

argon2_argon2id_body() {
	atf_check -s exit:0 -o match:"^\\\$argon2id\\\$" -x \
		'echo -n 'password' | argon2 somesalt -e -id'
}

atf_test_case argon2_argon2i
argon2_argon2i_head() {
	atf_set "descr" "ATF test for argon2 argon2i variant"
}

argon2_argon2i_body() {
	atf_check -s exit:0 -o match:"^\\\$argon2i\\\$" -x \
		'echo -n 'password' | argon2 somesalt -e -i'
}

atf_test_case argon2_argon2d
argon2_argon2d_head() {
	atf_set "descr" "ATF test for argon2 argon2d variant"
}

argon2_argon2d_body() {
	atf_check -s exit:0 -o match:"^\\\$argon2d\\\$" -x \
		'echo -n 'password' | argon2 somesalt -e -d'
}

atf_test_case argon2_argon2id_k2096_p2_t3
argon2_argon2id_k2096_p2_head() {
	atf_set "descr" "ATF test for argon2 argon2id,k=2096,p=2,t=3 "
}

argon2_argon2id_k2096_p2_t3_body() {
	atf_check -s exit:0 -o match:"^\\\$argon2id\\\$v=19\\\$m=2096,t=3,p=2" -x \
		'echo -n 'password' | argon2 somesalt -e -id -k 2096 -p 2 -t 3'
}

atf_test_case argon2_argon2i_k2096_p1_t4
argon2_argon2i_k2096_p1_t4_head() {
	atf_set "descr" "ATF test for argon2 argon2i,k=2096,p=1,t=4 "
}

argon2_argon2i_k2096_p1_t4_body() {
	atf_check -s exit:0 -o match:"^\\\$argon2i\\\$v=19\\\$m=2096,t=4,p=1" -x \
		'echo -n 'password' | argon2 somesalt -e -i -k 2096 -p 1 -t 4'
}

atf_test_case argon2_argon2d_k2096_p2_t4
argon2_argon2d_k2096_p2_t4_head() {
	atf_set "descr" "ATF test for argon2 argon2d,k=2096,p=2,t=4"
}

argon2_argon2d_k2096_p2_t4_body() {
	atf_check -s exit:0 -o match:"^\\\$argon2d\\\$v=19\\\$m=2096,t=4,p=2" -x \
		'echo -n 'password' | argon2 somesalt -e -d -k 2096 -p 2 -t 4'
}

atf_test_case argon2_argon2id_k2096_p1_v10
argon2_argon2id_k2096_p1_v10_head() {
	atf_set "descr" "ATF test for argon2 argon2id,k=2096,p=1,v=10"
}
argon2_argon2id_k2096_p1_v10_body() {
	atf_check -s exit:0 -o match:"^\\\$argon2id\\\$v=16\\\$m=2096,t=3,p=1" -x \
		'echo -n 'password' | argon2 somesalt -e -id -k 2096 -p 1 -v 10'
}

atf_test_case argon2_argon2id_k2096_p1_v13
argon2_argon2id_k2096_p1_v13_head() {
	atf_set "descr" "ATF test for argon2 argon2id,k=2096,p=1,v=13"
}
argon2_argon2id_k2096_p1_v13_body() {
	atf_check -s exit:0 -o match:"^\\\$argon2id\\\$v=19\\\$m=2096,t=3,p=1" -x \
		'echo -n 'password' | argon2 somesalt -e -id -k 2096 -p 1 -v 13'
}

atf_test_case argon2_argon2i_k2096_p1_v10
argon2_argon2i_k2096_p1_v10_head() {
	atf_set "descr" "ATF test for argon2 argon2i,k=2096,p=1,v=10"
}
argon2_argon2i_k2096_p1_v10_body() {
	atf_check -s exit:0 -o match:"^\\\$argon2i\\\$v=16\\\$m=2096,t=3,p=1" -x \
		'echo -n 'password' | argon2 somesalt -e -i -k 2096 -p 1 -v 10'
}

atf_test_case argon2_argon2i_k2096_p1_v13
argon2_argon2i_k2096_p1_v13_head() {
	atf_set "descr" "ATF test for argon2 argon2i,k=2096,p=1,v=13"
}
argon2_argon2i_k2096_p1_v13_body() {
	atf_check -s exit:0 -o match:"^\\\$argon2i\\\$v=19\\\$m=2096,t=3,p=1" -x \
		'echo -n 'password' | argon2 somesalt -e -i -k 2096 -p 1 -v 13'
}

atf_test_case argon2_argon2d_k2096_p1_v10
argon2_argon2d_k2096_p1_v10_head() {
	atf_set "descr" "ATF test for argon2 argon2d,k=2096,p=1,v=10"
}
argon2_argon2d_k2096_p1_v10_body() {
	atf_check -s exit:0 -o match:"^\\\$argon2d\\\$v=16\\\$m=2096,t=3,p=1" -x \
		'echo -n 'password' | argon2 somesalt -e -d -k 2096 -p 1 -v 10'
}

atf_test_case argon2_argon2d_k2096_p1_v13
argon2_argon2d_k2096_p1_v13_head() {
	atf_set "descr" "ATF test for argon2 argon2d,k=2096,p=1,v=13"
}
argon2_argon2d_k2096_p1_v13_body() {
	atf_check -s exit:0 -o match:"^\\\$argon2d\\\$v=19\\\$m=2096,t=3,p=1" -x \
		'echo -n 'password' | argon2 somesalt -e -d -k 2096 -p 1 -v 13'
}

atf_test_case argon2_argon2id_k2096_p1_v19_inver
argon2_argon2id_k2096_p1_v19_inver_head() {
	atf_set "descr" "ATF test for argon2 argon2d,k=2096,p=1 invalid version specification"
}
argon2_argon2id_k2096_p1_v19_inver_body() {
	atf_check -s exit:1 -e match:"Error: invalid Argon2 version" -x \
		'echo -n 'password' | argon2 somesalt -e -d -k 2096 -p 1 -v 19'
}

atf_test_case argon2_argon2id_k2096_p1_sts
argon2_argon2id_k2096_p1_sts_head() {
	atf_set "descr" "ATF test for argon2 argon2d,k=2096,p=1 salt too short"
}
argon2_argon2id_k2096_p1_sts_body() {
	atf_check -s exit:1 -e match:"Error: Salt is too short" -x \
		'echo -n 'password' | argon2 tshort -e -d -k 2096 -p 1'
}

atf_init_test_cases()
{
	atf_add_test_case argon2_argon2id
	atf_add_test_case argon2_argon2i
	atf_add_test_case argon2_argon2d
	atf_add_test_case argon2_argon2id_k2096_p2_t3
	atf_add_test_case argon2_argon2i_k2096_p1_t4
	atf_add_test_case argon2_argon2d_k2096_p2_t4
	atf_add_test_case argon2_argon2id_k2096_p1_v10
	atf_add_test_case argon2_argon2id_k2096_p1_v13
	atf_add_test_case argon2_argon2i_k2096_p1_v10
	atf_add_test_case argon2_argon2i_k2096_p1_v13
	atf_add_test_case argon2_argon2d_k2096_p1_v10
	atf_add_test_case argon2_argon2d_k2096_p1_v13
	atf_add_test_case argon2_argon2id_k2096_p1_v19_inver
	atf_add_test_case argon2_argon2id_k2096_p1_sts
}

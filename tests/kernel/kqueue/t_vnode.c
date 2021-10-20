#include <sys/event.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <atf-c.h>

/*
 * Test cases for events triggered by manipulating a target directory
 * content.  Using EVFILT_VNODE filter on the target directory descriptor.
 *
 */

static const char *dir_target = "foo";
static const char *dir_inside1 = "foo/bar1";
static const char *dir_inside2 = "foo/bar2";
static const char *dir_outside = "bar";
static const char *file_inside1 = "foo/baz1";
static const char *file_inside2 = "foo/baz2";
static const char *file_outside = "qux";
static const struct timespec ts = {0, 0};
static int kq = -1;
static int target = -1;

int init_target(void);
int init_kqueue(void);
int create_file(const char *);
void cleanup(void);

int
init_target(void)
{
	if (mkdir(dir_target, S_IRWXU) < 0) {
		return -1;
	}
	target = open(dir_target, O_RDONLY, 0);
	return target;
}

int
init_kqueue(void)
{
	struct kevent eventlist[1];

	kq = kqueue();
	if (kq < 0) {
		return -1;
	}
	EV_SET(&eventlist[0], (uintptr_t)target, EVFILT_VNODE,
		EV_ADD | EV_ONESHOT, NOTE_DELETE |
		NOTE_WRITE | NOTE_EXTEND | NOTE_ATTRIB |
		NOTE_LINK | NOTE_RENAME | NOTE_REVOKE, 0, 0);
	return kevent(kq, eventlist, 1, NULL, 0, NULL);
}

int
create_file(const char *file)
{
	int fd;

	fd = open(file, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
	if (fd < 0) {
		return -1;
	}
	return close(fd);
}

void
cleanup(void)
{
	(void)unlink(file_inside1);
	(void)unlink(file_inside2);
	(void)unlink(file_outside);
	(void)rmdir(dir_inside1);
	(void)rmdir(dir_inside2);
	(void)rmdir(dir_outside);
	(void)rmdir(dir_target);
	(void)close(kq);
	(void)close(target);
}

ATF_TC_WITH_CLEANUP(dir_no_note_link_create_file_in);
ATF_TC_HEAD(dir_no_note_link_create_file_in, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) does not return NOTE_LINK for the directory "
		"'foo' if a file 'foo/baz' is created.");
}
ATF_TC_BODY(dir_no_note_link_create_file_in, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(create_file(file_inside1) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_LINK, 0);
}
ATF_TC_CLEANUP(dir_no_note_link_create_file_in, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_no_note_link_delete_file_in);
ATF_TC_HEAD(dir_no_note_link_delete_file_in, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) does not return NOTE_LINK for the directory "
		"'foo' if a file 'foo/baz' is deleted.");
}
ATF_TC_BODY(dir_no_note_link_delete_file_in, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(create_file(file_inside1) != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(unlink(file_inside1) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_LINK, 0);
}
ATF_TC_CLEANUP(dir_no_note_link_delete_file_in, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_no_note_link_mv_dir_within);
ATF_TC_HEAD(dir_no_note_link_mv_dir_within, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) does not return NOTE_LINK for the directory "
		"'foo' if a directory 'foo/bar' is renamed to 'foo/baz'.");
}
ATF_TC_BODY(dir_no_note_link_mv_dir_within, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(mkdir(dir_inside1, S_IRWXU) != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(rename(dir_inside1, dir_inside2) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_LINK, 0);
}
ATF_TC_CLEANUP(dir_no_note_link_mv_dir_within, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_no_note_link_mv_file_within);
ATF_TC_HEAD(dir_no_note_link_mv_file_within, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) does not return NOTE_LINK for the directory "
		"'foo' if a file 'foo/baz' is renamed to 'foo/qux'.");
}
ATF_TC_BODY(dir_no_note_link_mv_file_within, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(create_file(file_inside1) != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(rename(file_inside1, file_inside2) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_LINK, 0);
}
ATF_TC_CLEANUP(dir_no_note_link_mv_file_within, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_note_link_create_dir_in);
ATF_TC_HEAD(dir_note_link_create_dir_in, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) returns NOTE_LINK for the directory "
		"'foo' if a directory 'foo/bar' is created.");
}
ATF_TC_BODY(dir_note_link_create_dir_in, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(mkdir(dir_inside1, S_IRWXU) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_LINK, NOTE_LINK);
}
ATF_TC_CLEANUP(dir_note_link_create_dir_in, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_note_link_delete_dir_in);
ATF_TC_HEAD(dir_note_link_delete_dir_in, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) returns NOTE_LINK for the directory "
		"'foo' if a directory 'foo/bar' is deleted.");
}
ATF_TC_BODY(dir_note_link_delete_dir_in, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(mkdir(dir_inside1, S_IRWXU) != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(rmdir(dir_inside1) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_LINK, NOTE_LINK);
}
ATF_TC_CLEANUP(dir_note_link_delete_dir_in, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_note_link_mv_dir_in);
ATF_TC_HEAD(dir_note_link_mv_dir_in, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) returns NOTE_LINK for the directory "
		"'foo' if a directory 'bar' is renamed to 'foo/bar'.");
}
ATF_TC_BODY(dir_note_link_mv_dir_in, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(mkdir(dir_outside, S_IRWXU) != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(rename(dir_outside, dir_inside1) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_LINK, NOTE_LINK);
}
ATF_TC_CLEANUP(dir_note_link_mv_dir_in, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_note_link_mv_dir_out);
ATF_TC_HEAD(dir_note_link_mv_dir_out, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) returns NOTE_LINK for the directory "
		"'foo' if a directory 'foo/bar' is renamed to 'bar'.");
}
ATF_TC_BODY(dir_note_link_mv_dir_out, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(mkdir(dir_inside1, S_IRWXU) != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(rename(dir_inside1, dir_outside) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_LINK, NOTE_LINK);
}
ATF_TC_CLEANUP(dir_note_link_mv_dir_out, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_note_write_create_dir_in);
ATF_TC_HEAD(dir_note_write_create_dir_in, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) returns NOTE_WRITE for the directory "
		"'foo' if a directory 'foo/bar' is created.");
}
ATF_TC_BODY(dir_note_write_create_dir_in, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(mkdir(dir_inside1, S_IRWXU) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_WRITE, NOTE_WRITE);
}
ATF_TC_CLEANUP(dir_note_write_create_dir_in, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_note_write_create_file_in);
ATF_TC_HEAD(dir_note_write_create_file_in, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) returns NOTE_WRITE for the directory "
		"'foo' if a file 'foo/baz' is created.");
}
ATF_TC_BODY(dir_note_write_create_file_in, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(create_file(file_inside1) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_WRITE, NOTE_WRITE);
}
ATF_TC_CLEANUP(dir_note_write_create_file_in, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_note_write_delete_dir_in);
ATF_TC_HEAD(dir_note_write_delete_dir_in, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) returns NOTE_WRITE for the directory "
		"'foo' if a directory 'foo/bar' is deleted.");
}
ATF_TC_BODY(dir_note_write_delete_dir_in, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(mkdir(dir_inside1, S_IRWXU) != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(rmdir(dir_inside1) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_WRITE, NOTE_WRITE);
}
ATF_TC_CLEANUP(dir_note_write_delete_dir_in, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_note_write_delete_file_in);
ATF_TC_HEAD(dir_note_write_delete_file_in, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) returns NOTE_WRITE for the directory "
		"'foo' if a file 'foo/baz' is deleted.");
}
ATF_TC_BODY(dir_note_write_delete_file_in, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(create_file(file_inside1) != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(unlink(file_inside1) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_WRITE, NOTE_WRITE);
}
ATF_TC_CLEANUP(dir_note_write_delete_file_in, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_note_write_mv_dir_in);
ATF_TC_HEAD(dir_note_write_mv_dir_in, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) returns NOTE_WRITE for the directory "
		"'foo' if a directory 'bar' is renamed to 'foo/bar'.");
}
ATF_TC_BODY(dir_note_write_mv_dir_in, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(mkdir(dir_outside, S_IRWXU) != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(rename(dir_outside, dir_inside1) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_WRITE, NOTE_WRITE);
}
ATF_TC_CLEANUP(dir_note_write_mv_dir_in, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_note_write_mv_dir_out);
ATF_TC_HEAD(dir_note_write_mv_dir_out, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) returns NOTE_WRITE for the directory "
		"'foo' if a directory 'foo/bar' is renamed to 'bar'.");
}
ATF_TC_BODY(dir_note_write_mv_dir_out, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(mkdir(dir_inside1, S_IRWXU) != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(rename(dir_inside1, dir_outside) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_WRITE, NOTE_WRITE);
}
ATF_TC_CLEANUP(dir_note_write_mv_dir_out, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_note_write_mv_dir_within);
ATF_TC_HEAD(dir_note_write_mv_dir_within, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) returns NOTE_WRITE for the directory "
		"'foo' if a directory 'foo/bar' is renamed to 'foo/baz'.");
}
ATF_TC_BODY(dir_note_write_mv_dir_within, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(mkdir(dir_inside1, S_IRWXU) != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(rename(dir_inside1, dir_inside2) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_WRITE, NOTE_WRITE);
}
ATF_TC_CLEANUP(dir_note_write_mv_dir_within, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_note_write_mv_file_in);
ATF_TC_HEAD(dir_note_write_mv_file_in, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) returns NOTE_WRITE for the directory "
		"'foo' if a file 'qux' is renamed to 'foo/baz'.");
}
ATF_TC_BODY(dir_note_write_mv_file_in, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(create_file(file_outside) != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(rename(file_outside, file_inside1) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_WRITE, NOTE_WRITE);
}
ATF_TC_CLEANUP(dir_note_write_mv_file_in, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_note_write_mv_file_out);
ATF_TC_HEAD(dir_note_write_mv_file_out, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) returns NOTE_WRITE for the directory "
		"'foo' if a file 'foo/baz' is renamed to 'qux'.");
}
ATF_TC_BODY(dir_note_write_mv_file_out, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(create_file(file_inside1) != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(rename(file_inside1, file_outside) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_WRITE, NOTE_WRITE);
}
ATF_TC_CLEANUP(dir_note_write_mv_file_out, tc)
{
	cleanup();
}

ATF_TC_WITH_CLEANUP(dir_note_write_mv_file_within);
ATF_TC_HEAD(dir_note_write_mv_file_within, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case ensures "
		"that kevent(2) returns NOTE_WRITE for the directory "
		"'foo' if a file 'foo/baz' is renamed to 'foo/qux'.");
}
ATF_TC_BODY(dir_note_write_mv_file_within, tc)
{
	struct kevent changelist[1];

	ATF_REQUIRE(init_target() != -1);
	ATF_REQUIRE(create_file(file_inside1) != -1);
	ATF_REQUIRE(init_kqueue() != -1);

	ATF_REQUIRE(rename(file_inside1, file_inside2) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, changelist, 1, &ts) != -1);
	ATF_CHECK_EQ(changelist[0].fflags & NOTE_WRITE, NOTE_WRITE);
}
ATF_TC_CLEANUP(dir_note_write_mv_file_within, tc)
{
	cleanup();
}

static const char testfile[] = "testfile";

ATF_TC_WITH_CLEANUP(open_write_read_close);
ATF_TC_HEAD(open_write_read_close, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case exercises "
		"that kevent(2) returns NOTE_OPEN, NOTE_READ, NOTE_WRITE, "
		"NOTE_CLOSE, and NOTE_CLOSE_WRITE.");
}
ATF_TC_BODY(open_write_read_close, tc)
{
	struct kevent event[1];
	char buf[sizeof(testfile)];
	int fd;

	ATF_REQUIRE((kq = kqueue()) != -1);

	/*
	 * Create the test file and register an event on it.  We need
	 * to keep the fd open to keep receiving events, so we'll just
	 * leak it and re-use the fd variable.
	 */
	ATF_REQUIRE((fd = open(testfile,
			       O_RDWR | O_CREAT | O_TRUNC, 0600)) != -1);
	EV_SET(&event[0], fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
	       NOTE_OPEN | NOTE_READ | NOTE_WRITE |
	       NOTE_CLOSE | NOTE_CLOSE_WRITE, 0, NULL);
	ATF_REQUIRE(kevent(kq, event, 1, NULL, 0, NULL) == 0);

	/*
	 * Open the file for writing, check for NOTE_OPEN.
	 * Write to the file, check for NOTE_WRITE | NOTE_EXTEND.
	 * Re-write the file, check for NOTE_WRITE and !NOTE_EXTEND.
	 * Write one additional byte, check for NOTE_WRITE | NOTE_EXTEND.
	 * Close the file, check for NOTE_CLOSE_WRITE.
	 */
	ATF_REQUIRE((fd = open(testfile, O_RDWR)) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
	ATF_REQUIRE(event[0].fflags & NOTE_OPEN);

	ATF_REQUIRE((pwrite(fd, testfile,
			    sizeof(testfile), 0)) == sizeof(testfile));
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
	ATF_REQUIRE(event[0].fflags & NOTE_WRITE);
	ATF_REQUIRE(event[0].fflags & NOTE_EXTEND);

	ATF_REQUIRE((pwrite(fd, testfile,
			    sizeof(testfile), 0)) == sizeof(testfile));
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
	ATF_REQUIRE(event[0].fflags & NOTE_WRITE);
	ATF_REQUIRE((event[0].fflags & NOTE_EXTEND) == 0);

	ATF_REQUIRE((pwrite(fd, testfile,
			    1, sizeof(testfile))) == 1);
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
	ATF_REQUIRE(event[0].fflags & NOTE_WRITE);
	ATF_REQUIRE(event[0].fflags & NOTE_EXTEND);

	(void)close(fd);
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
	ATF_REQUIRE(event[0].fflags & NOTE_CLOSE_WRITE);
	ATF_REQUIRE((event[0].fflags & NOTE_CLOSE) == 0);

	/*
	 * Open the file for reading, check for NOTE_OPEN.
	 * Read from the file, check for NOTE_READ.
	 * Close the file., check for NOTE_CLOSE.
	 */
	ATF_REQUIRE((fd = open(testfile, O_RDONLY)) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
	ATF_REQUIRE(event[0].fflags & NOTE_OPEN);

	ATF_REQUIRE((read(fd, buf, sizeof(buf))) == sizeof(buf));
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
	ATF_REQUIRE(event[0].fflags & NOTE_READ);

	(void)close(fd);
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
	ATF_REQUIRE(event[0].fflags & NOTE_CLOSE);
	ATF_REQUIRE((event[0].fflags & NOTE_CLOSE_WRITE) == 0);
}
ATF_TC_CLEANUP(open_write_read_close, tc)
{
	(void)unlink(testfile);
}

ATF_TC_WITH_CLEANUP(interest);
ATF_TC_HEAD(interest, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case exercises "
		"the kernel code that computes vnode kevent interest");
}
ATF_TC_BODY(interest, tc)
{
	struct kevent event[3];
	int open_ev_fd, write_ev_fd, close_ev_fd;
	int fd;

	/*
	 * This test cases exercises some implementation details
	 * regarding how "kevent interest" is computed for a vnode.
	 *
	 * We are going to add events, one at a time, in a specific
	 * order, and then remove one of them, with the knowledge that
	 * a specific code path in vfs_vnops.c:vn_knote_detach() will
	 * be taken.  There are several KASSERT()s in this code path
	 * that will be validated.
	 *
	 * In order to ensure distinct knotes are attached to the vnodes,
	 * we must use a different file descriptor to register interest
	 * in each kind of event.
	 */

	ATF_REQUIRE((kq = kqueue()) != -1);

	/*
	 * Create the test file and register an event on it.  We need
	 * to keep the fd open to keep receiving events, so we'll just
	 * leak it and re-use the fd variable.
	 */
	ATF_REQUIRE((open_ev_fd = open(testfile,
	    O_RDWR | O_CREAT | O_TRUNC, 0600)) != -1);
	ATF_REQUIRE((write_ev_fd = dup(open_ev_fd)) != -1);
	ATF_REQUIRE((close_ev_fd = dup(open_ev_fd)) != -1);

	EV_SET(&event[0], open_ev_fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
	       NOTE_OPEN, 0, NULL);
	EV_SET(&event[1], write_ev_fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
	       NOTE_WRITE, 0, NULL);
	EV_SET(&event[2], close_ev_fd, EVFILT_VNODE, EV_ADD | EV_CLEAR,
	       NOTE_CLOSE | NOTE_CLOSE_WRITE, 0, NULL);
	ATF_REQUIRE(kevent(kq, event, 3, NULL, 0, NULL) == 0);

	/*
	 * The testfile vnode now has 3 knotes attached, in "LIFO"
	 * order:
	 *
	 *	NOTE_CLOSE -> NOTE_WRITE -> NOTE_OPEN
	 *
	 * We will now remove the NOTE_WRITE knote.
	 */
	(void)close(write_ev_fd);

	ATF_REQUIRE((fd = open(testfile, O_RDWR)) != -1);
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
	ATF_REQUIRE(event[0].fflags & NOTE_OPEN);

	ATF_REQUIRE((pwrite(fd, testfile,
			    sizeof(testfile), 0)) == sizeof(testfile));
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 0);

	(void)close(fd);
	ATF_REQUIRE(kevent(kq, NULL, 0, event, 1, &ts) == 1);
	ATF_REQUIRE(event[0].fflags & NOTE_CLOSE_WRITE);
	ATF_REQUIRE((event[0].fflags & NOTE_CLOSE) == 0);

}
ATF_TC_CLEANUP(interest, tc)
{
	(void)unlink(testfile);
}

ATF_TC_WITH_CLEANUP(rename_over_self_hardlink);
ATF_TC_HEAD(rename_over_self_hardlink, tc)
{
	atf_tc_set_md_var(tc, "descr", "This test case tests "
		"renaming a file over a hard-link to itself");
}
ATF_TC_BODY(rename_over_self_hardlink, tc)
{
	struct kevent event[2], *dir_ev, *file_ev;
	int dir_fd, file_fd;

	ATF_REQUIRE((kq = kqueue()) != -1);

	ATF_REQUIRE((mkdir(dir_target, 0700)) == 0);
	ATF_REQUIRE((dir_fd = open(dir_target, O_RDONLY)) != -1);

	ATF_REQUIRE((file_fd = open(file_inside1, O_RDONLY | O_CREAT,
	    0600)) != -1);
	ATF_REQUIRE(link(file_inside1, file_inside2) == 0);

	EV_SET(&event[0], dir_fd, EVFILT_VNODE, EV_ADD,
	    NOTE_WRITE | NOTE_EXTEND | NOTE_LINK, 0, NULL);
	EV_SET(&event[1], file_fd, EVFILT_VNODE, EV_ADD,
	    NOTE_LINK | NOTE_DELETE, 0, NULL);
	ATF_REQUIRE(kevent(kq, event, 2, NULL, 0, NULL) == 0);

	ATF_REQUIRE(rename(file_inside1, file_inside2) == 0);

	ATF_REQUIRE(kevent(kq, NULL, 0, event, 2, &ts) == 2);
	ATF_REQUIRE(event[0].ident == (uintptr_t)dir_fd ||
		    event[0].ident == (uintptr_t)file_fd);
	ATF_REQUIRE(event[1].ident == (uintptr_t)dir_fd ||
		    event[1].ident == (uintptr_t)file_fd);
	if (event[0].ident == (uintptr_t)dir_fd) {
		dir_ev = &event[0];
		file_ev = &event[1];
	} else {
		dir_ev = &event[1];
		file_ev = &event[0];
	}
	ATF_REQUIRE(dir_ev->fflags == NOTE_WRITE);
	ATF_REQUIRE(file_ev->fflags == NOTE_LINK);
}
ATF_TC_CLEANUP(rename_over_self_hardlink, tc)
{
	cleanup();
}

ATF_TP_ADD_TCS(tp)
{
	ATF_TP_ADD_TC(tp, dir_no_note_link_create_file_in);
	ATF_TP_ADD_TC(tp, dir_no_note_link_delete_file_in);

	ATF_TP_ADD_TC(tp, dir_no_note_link_mv_dir_within);
	ATF_TP_ADD_TC(tp, dir_no_note_link_mv_file_within);

	ATF_TP_ADD_TC(tp, dir_note_link_create_dir_in);
	ATF_TP_ADD_TC(tp, dir_note_link_delete_dir_in);

	ATF_TP_ADD_TC(tp, dir_note_link_mv_dir_in);
	ATF_TP_ADD_TC(tp, dir_note_link_mv_dir_out);

	ATF_TP_ADD_TC(tp, dir_note_write_create_dir_in);
	ATF_TP_ADD_TC(tp, dir_note_write_create_file_in);

	ATF_TP_ADD_TC(tp, dir_note_write_delete_dir_in);
	ATF_TP_ADD_TC(tp, dir_note_write_delete_file_in);

	ATF_TP_ADD_TC(tp, dir_note_write_mv_dir_in);
	ATF_TP_ADD_TC(tp, dir_note_write_mv_dir_out);
	ATF_TP_ADD_TC(tp, dir_note_write_mv_dir_within);
	ATF_TP_ADD_TC(tp, dir_note_write_mv_file_in);
	ATF_TP_ADD_TC(tp, dir_note_write_mv_file_out);
	ATF_TP_ADD_TC(tp, dir_note_write_mv_file_within);

	ATF_TP_ADD_TC(tp, rename_over_self_hardlink);

	ATF_TP_ADD_TC(tp, open_write_read_close);
	ATF_TP_ADD_TC(tp, interest);

	return atf_no_error();
}

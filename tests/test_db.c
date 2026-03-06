#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include <cmocka.h>
#include "db/db.h"


// Assert we can initialize an empty database in memory.
static void test_db_init() {
    assert_int_equal(db_init(":memory:"), 0);
}


// Assert we can initialize an empty database on disk.
static void test_db_init_2() {
    // Remove file if it already exists.
    const char *db = "/tmp/foo.db";
    unlink(db);

    // Create database at the path.
    assert_int_equal(db_init(db), 0);

    // Check that db file exists and then remove it.
    struct stat st;
    assert_int_equal(stat(db, &st), 0);
    assert_true(S_ISREG(st.st_mode));
    unlink(db);
}

// Assert we can retrieve top files from an empty database.
static void test_db_get_top_empty() {
    // Initialize database.
    const char *db = "/tmp/foo.db";
    assert_int_equal(db_init(db), 0);

    // Retrieve a (hopefully) empty list of results.
    char **paths;
    int count = 0;
    int rc = db_get_top(db, 10, &paths, &count);
    assert_int_equal(rc, 0);
    assert_int_equal(count, 0);

    unlink(db);
}


// Assert we can insert a few files and retrieve them in the expected order.
static void test_db_insert_and_get() {
    // Initialize database.
    const time_t now = time(NULL);
    const char *db = "/tmp/foo.db";
    assert_int_equal(db_init(db), 0);

    int rc;

    rc = db_update_file(db, "a", now - 20);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "b", now - 19);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "d", now - 18);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "c", now + 0);
    assert_int_equal(rc, 0);

    char **paths;
    int count = 0;
    rc = db_get_top(db, 10, &paths, &count);
    assert_int_equal(rc, 0);
    assert_int_equal(count, 4);

    assert_string_equal(paths[0], "c");
    assert_string_equal(paths[1], "d");
    assert_string_equal(paths[2], "b");
    assert_string_equal(paths[3], "a");

    unlink(db);
}

// Assert we can limit large results to smaller ones.
static void test_db_limit() {
    // Initialize database.
    const time_t now = time(NULL);
    const char *db = "/tmp/foo.db";
    assert_int_equal(db_init(db), 0);

    int rc;

    rc = db_update_file(db, "a", now - 2);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "b", now - 1);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "c", now - 0);
    assert_int_equal(rc, 0);

    char **paths;
    int count = 0;
    rc = db_get_top(db, 2, &paths, &count);
    assert_int_equal(rc, 0);
    assert_int_equal(count, 2);

    assert_string_equal(paths[0], "c");
    assert_string_equal(paths[1], "b");

    unlink(db);
}


// Assert idempotency.
static void test_db_idempotent() {
    // Initialize database.
    const time_t now = time(NULL);
    const char *db = "/tmp/foo.db";
    assert_int_equal(db_init(db), 0);

    int rc;

    rc = db_update_file(db, "a", now - 2);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "b", now - 1);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "a", now - 0);
    assert_int_equal(rc, 0);

    char **paths;
    int count = 0;
    rc = db_get_top(db, 10, &paths, &count);
    assert_int_equal(rc, 0);
    assert_int_equal(count, 2);

    assert_string_equal(paths[0], "a");
    assert_string_equal(paths[1], "b");

    unlink(db);
}

// Assert many counts makes a file rise to the top.
static void test_db_many_count() {
    // Initialize database.
    const time_t now = time(NULL);
    const char *db = "/tmp/foo.db";
    assert_int_equal(db_init(db), 0);

    int rc;

    for (int i = 0; i < 100; i++) {
        rc = db_update_file(db, "a", now + i - 1000);
        assert_int_equal(rc, 0);
    }
    rc = db_update_file(db, "b", now + 0);
    assert_int_equal(rc, 0);

    char **paths;
    int count = 0;
    rc = db_get_top(db, 10, &paths, &count);
    assert_int_equal(rc, 0);
    assert_int_equal(count, 2);

    assert_string_equal(paths[0], "a");
    assert_string_equal(paths[1], "b");

    unlink(db);
}


// Assert we gracefully handle non-writable paths.
static void test_db_nonwritable() {
    // Initialize database.
    const char *db = "/foo.db";
    assert_int_not_equal(db_init(db), 0);
}

// Assert we can perform "normal" operations with expected results.
static void test_normal() {
    // Initialize database.
    const time_t now = time(NULL);
    const char *db = "/tmp/foo.db";
    assert_int_equal(db_init(db), 0);
    int rc;

    // Insert a few files.
    rc = db_update_file(db, "a", now - 128000);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "b", now - 127000);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "a", now - 116000);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "c", now - 115000);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "b", now - 114000);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "b", now - 113000);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "c", now - 112000);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "d", now - 1000);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "c", now - 0000);
    assert_int_equal(rc, 0);

    // Assert on the order of them.
    char **paths;
    int count = 0;
    rc = db_get_top(db, 10, &paths, &count);
    assert_int_equal(rc, 0);
    assert_int_equal(count, 4);
    assert_string_equal(paths[0], "c");
    assert_string_equal(paths[1], "d");
    assert_string_equal(paths[2], "b");
    assert_string_equal(paths[3], "a");
}

int main() {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_db_init),
        cmocka_unit_test(test_db_init_2),
        cmocka_unit_test(test_db_get_top_empty),
        cmocka_unit_test(test_db_insert_and_get),
        cmocka_unit_test(test_db_limit),
        cmocka_unit_test(test_db_idempotent),
        cmocka_unit_test(test_db_nonwritable),
        cmocka_unit_test(test_db_many_count),
        cmocka_unit_test(test_normal),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}

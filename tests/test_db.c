#include <stddef.h>
#include <stdarg.h>
#include <setjmp.h>
#include <sys/stat.h>
#include <unistd.h>
#include <cmocka.h>
#include "db/db.h"

#define TIMESTAMP 1772412200

// Assert we can initialize an empty database in memory.
static void test_db_init() {
    db_t *db = db_init(":memory:");
    assert_non_null(db);
}


// Assert we can initialize an empty database on disk.
static void test_db_init_2() {
    // Remove file if it already exists.
    const char *path = "/tmp/foo.db";
    unlink(path);

    // Create database at the path.
    db_t *db = db_init(path);
    assert_non_null(db);

    // Check that db file exists and then remove it.
    struct stat st;
    assert_int_equal(stat(path, &st), 0);
    assert_true(S_ISREG(st.st_mode));
    unlink(path);
}

// Assert we can retrieve top files from an empty database.
static void test_db_get_top_empty() {
    // Initialize database.
    const char *path = "/tmp/foo.db";
    db_t *db = db_init(path);
    assert_non_null(db);

    // Retrieve a (hopefully) empty list of results.
    db_file_t *files;
    int count = 0;
    int rc = db_get_top(db, 10, &files, &count);
    assert_int_equal(rc, 0);
    assert_int_equal(count, 0);

    unlink(path);
}


// Assert we can insert a few files and retrieve them in the expected order.
static void test_db_insert_and_get() {
    // Initialize database.
    const char *path = "/tmp/foo.db";
    db_t *db = db_init(path);
    assert_non_null(db);

    int rc;

    rc = db_update_file(db, "a", TIMESTAMP + 0);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "b", TIMESTAMP + 1);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "c", TIMESTAMP + 20);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "d", TIMESTAMP + 2);
    assert_int_equal(rc, 0);

    db_file_t *files;
    int count = 0;
    rc = db_get_top(db, 10, &files, &count);
    assert_int_equal(rc, 0);
    assert_int_equal(count, 4);

    assert_string_equal(files[0].path, "c");
    assert_string_equal(files[1].path, "d");
    assert_string_equal(files[2].path, "b");
    assert_string_equal(files[3].path, "a");

    unlink(path);
}

// Assert we can limit large results to smaller ones.
static void test_db_limit() {
    // Initialize database.
    const char *path = "/tmp/foo.db";
    db_t *db = db_init(path);
    assert_non_null(db);

    int rc;

    rc = db_update_file(db, "a", TIMESTAMP + 0);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "b", TIMESTAMP + 1);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "c", TIMESTAMP + 2);
    assert_int_equal(rc, 0);

    db_file_t *files;
    int count = 0;
    rc = db_get_top(db, 2, &files, &count);
    assert_int_equal(rc, 0);
    assert_int_equal(count, 2);

    assert_string_equal(files[0].path, "c");
    assert_string_equal(files[1].path, "b");

    unlink(path);
}


// Assert idempotency.
static void test_db_idempotent() {
    // Initialize database.
    const char *path = "/tmp/foo.db";
    db_t *db = db_init(path);
    assert_non_null(db);

    int rc;

    rc = db_update_file(db, "a", TIMESTAMP + 0);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "b", TIMESTAMP + 1);
    assert_int_equal(rc, 0);
    rc = db_update_file(db, "a", TIMESTAMP + 2);
    assert_int_equal(rc, 0);

    db_file_t *files;
    int count = 0;
    rc = db_get_top(db, 10, &files, &count);
    assert_int_equal(rc, 0);
    assert_int_equal(count, 2);

    assert_string_equal(files[0].path, "a");
    assert_string_equal(files[1].path, "b");

    unlink(path);
}

// Assert we gracefully handle non-writable paths.
static void test_db_nonwritable() {
    db_t *db = db_init("/foo");
    assert_null(db);
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
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}

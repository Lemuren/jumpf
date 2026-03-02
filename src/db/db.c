#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sqlite3.h>
#include <string.h>
#include "db/db.h"

struct db {
    sqlite3 *conn;
    char *path;
};

static const char *schema =
#include "db/schema.sql"
;

static void _close(db_t *d, sqlite3_stmt *stmt);
static double _calculate_score(int atime, int count);


db_t *db_init(const char *path) {
    // Allocate database struct.
    db_t *d = malloc(sizeof *d);
    if (!d) return NULL;
    // TODO: Replace unsafe strcpy call.
    d->path = calloc(2048, sizeof (char));
    strcpy(d->path, path);

    // Open a connection.
    if (sqlite3_open(d->path, &d->conn) != SQLITE_OK) {
        free(d->path);
        free(d);
        return NULL;
    }

    // Use the schema to initialize the database.
    if (sqlite3_exec(d->conn, schema, NULL, NULL, NULL) != SQLITE_OK) {
        _close(d, NULL);
        free(d->path);
        free(d);
        return NULL;
    }

    sqlite3_close(d->conn);
    return d;
}


// Return files based on score. Highest first.
int db_get_top(db_t *d, int limit, db_file_t **files, int *count) {
    // Open a database connection.
    if (sqlite3_open(d->path, &d->conn) != SQLITE_OK) return 1;
    sqlite3_stmt *stmt = NULL;

    // Prepare and bind statement.
    const char *sql =
        " SELECT path, atime, count, score FROM files "
        " ORDER BY score DESC "
        " LIMIT ? ; "
    ;
    if (sqlite3_prepare_v2(d->conn, sql, -1, &stmt, NULL) != SQLITE_OK) {
        _close(d, stmt);
        return 2;
    }

    if (sqlite3_bind_int(stmt, 1, limit) != SQLITE_OK) {
        _close(d, stmt);
        return 3;
    }

    // Allocate and fill array of structs.
    *files = calloc(limit, sizeof **files);
    *count = 0;
    for (*count = 0; sqlite3_step(stmt) == SQLITE_ROW; (*count)++) {
        // TODO: Replace unsafe strcpy.
        (*files)[*count].path = calloc(2048, sizeof (char));
        strcpy((*files)[*count].path, (char *)sqlite3_column_text(stmt, 0));
        (*files)[*count].atime = sqlite3_column_int64(stmt, 1);
        (*files)[*count].count = sqlite3_column_int64(stmt, 2);
        (*files)[*count].score = sqlite3_column_double(stmt, 3);
    }

    return 0;
}


int db_update_file(db_t *d, const char *path, int atime) {
    // Open a database connection.
    if (sqlite3_open(d->path, &d->conn) != SQLITE_OK) return 1;
    sqlite3_stmt *stmt = NULL;

    // Prepare and bind statement.
    const char *sql =
        " SELECT path, atime, count, score FROM files "
        " WHERE path = ?; "
    ;
    if (sqlite3_prepare_v2(d->conn, sql, -1, &stmt, NULL) != SQLITE_OK) {
        _close(d, stmt);
        return 2;
    }

    if (sqlite3_bind_text(stmt, 1, path, -1, NULL) != SQLITE_OK) {
        _close(d, stmt);
        return 3;
    }

    // Fetch file information from the database.
    int rc = sqlite3_step(stmt);
    // If it already exists we update the file.
    if (rc == SQLITE_ROW) {
        int count = sqlite3_column_int64(stmt, 2) + 1;
        double score = _calculate_score(atime, count);
        const char *sql =
            " UPDATE files "
            " SET atime = ?, count = ?, score = ? "
            " WHERE path = ? ; "
        ;

        if (sqlite3_prepare_v2(d->conn, sql, -1, &stmt, NULL) != SQLITE_OK) {
            _close(d, stmt);
            return 10;
        }

        if (sqlite3_bind_int(stmt, 1, atime) != SQLITE_OK) {
            _close(d, stmt);
            return 11;
        }

        if (sqlite3_bind_int(stmt, 2, count) != SQLITE_OK) {
            _close(d, stmt);
            return 12;
        }

        if (sqlite3_bind_double(stmt, 3, score) != SQLITE_OK) {
            _close(d, stmt);
            return 13;
        }

        if (sqlite3_bind_text(stmt, 4, path, -1, NULL) != SQLITE_OK) {
            _close(d, stmt);
            return 14;
        }

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            _close(d, stmt);
            return 15;
        }

    // If it doesn't exist we create it.
    } else if (rc == SQLITE_DONE) {
        double score = _calculate_score(atime, 1);
        const char *sql =
            " INSERT INTO files (path, atime, count, score) "
            " VALUES (?, ?, 1, ?) ; "
        ;
        if (sqlite3_prepare_v2(d->conn, sql, -1, &stmt, NULL) != SQLITE_OK) {
            _close(d, stmt);
            return 4;
        }

        if (sqlite3_bind_text(stmt, 1, path, -1, NULL) != SQLITE_OK) {
            _close(d, stmt);
            return 5;
        }

        if (sqlite3_bind_int(stmt, 2, atime) != SQLITE_OK) {
            _close(d, stmt);
            return 6;
        }

        if (sqlite3_bind_double(stmt, 3, score) != SQLITE_OK) {
            _close(d, stmt);
            return 7;
        }

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            _close(d, stmt);
            return 8;
        }


    // Anything else is an error.
    } else {
        _close(d, stmt);
        return 9;
    }

    _close(d, stmt);
    return 0;
}

void db_free_results(db_file_t* files, int count) {
    for (int i = 0; i < count; i++) {
        free(files[i].path);
    }
    free(files);
}

// #### Private Methods ####
static void _close(db_t *d, sqlite3_stmt *stmt) {
    sqlite3_finalize(stmt);
    sqlite3_close(d->conn);
}

double _calculate_score(int atime, int count) {
    time_t now = time(NULL);
    double dt = (double)(now - atime);
    double lambda = -0.00001;
    return count * exp(lambda * dt);
}

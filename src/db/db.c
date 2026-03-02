#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sqlite3.h>
#include <string.h>
#include <stdbool.h>
#include "db/db.h"

#define FILE_EXISTS 0
#define FILE_NO_EXISTS -1

struct db {
    sqlite3 *conn;
    char *path;
};

static const char *schema =
#include "db/schema.sql"
;

static void _close(db_t *d, sqlite3_stmt *stmt);
static double _calculate_score(int atime, int count);
static int _write(db_t *d, db_file_t file, bool update);
static int _read(db_t *d, const char *path, db_file_t *file);


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

    _close(d, stmt);
    return 0;
}


int db_update_file(db_t *d, const char *path, int atime) {
    db_file_t file = { 0 };
    int rc = _read(d, path, &file);

    if (rc == FILE_EXISTS) {
        file.count++;
        file.atime = atime;
        file.score = _calculate_score(atime, file.count);
        if (_write(d, file, true)) return 2;
    } else if (rc == FILE_NO_EXISTS) {
        file.path = calloc(2048, sizeof (char));
        strcpy(file.path, path);
        file.count = 1;
        file.atime = atime;
        file.score = _calculate_score(file.atime, file.count);
        if (_write(d, file, false)) return 3;
    } else {
        return 1;
    }

    return 0;
}

void db_free_results(db_file_t* files, int count) {
    for (int i = 0; i < count; i++) {
        free(files[i].path);
    }
    free(files);
}


// #### Private Methods ####

static int _read(db_t *d, const char *path, db_file_t *file) {
    if (sqlite3_open(d->path, &d->conn) != SQLITE_OK) return 1;
    sqlite3_stmt *stmt = NULL;

    // Prepare and bind statement.
    const char *sql =
        " SELECT path, atime, count, score FROM files "
        " WHERE path = ?; "
    ;
    if (sqlite3_prepare_v2(d->conn, sql, -1, &stmt, NULL) != SQLITE_OK) goto fail;
    if (sqlite3_bind_text(stmt, 1, path, -1, NULL) != SQLITE_OK) goto fail;

    // Fetch file from the database.
    int rc = sqlite3_step(stmt);
    if (rc == SQLITE_ROW) {
        file->path = calloc(2048, sizeof (char));
        strcpy(file->path, (char *)sqlite3_column_text(stmt, 0));
        file->atime = sqlite3_column_int64(stmt, 1);
        file->count = sqlite3_column_int64(stmt, 2);
        file->score = sqlite3_column_double(stmt, 3);

        sqlite3_finalize(stmt);
        sqlite3_close(d->conn);
        return FILE_EXISTS;
    } else if (rc == SQLITE_DONE) {
        file->path = NULL;
        sqlite3_finalize(stmt);
        sqlite3_close(d->conn);
        return FILE_NO_EXISTS;
    } else {
        goto fail;
    }

    fail:
        sqlite3_finalize(stmt);
        sqlite3_close(d->conn);
        return 1;
}

static int _write(db_t *d, db_file_t file, bool update) {
    if (sqlite3_open(d->path, &d->conn) != SQLITE_OK) return 1;
    sqlite3_stmt *stmt = NULL;

    const char *sql = update ?
        " UPDATE files "
        " SET path = ?, atime = ?, count = ?, score = ? "
        " WHERE path = ? ; "
        :
        " INSERT INTO files (path, atime, count, score) "
        " VALUES (?, ?, ?, ?) ; "
    ;

    if (sqlite3_prepare_v2(d->conn, sql, -1, &stmt, NULL) != SQLITE_OK) goto fail;
    if (sqlite3_bind_text(stmt, 1, file.path, -1, NULL) != SQLITE_OK) goto fail;
    if (sqlite3_bind_int(stmt, 2, file.atime) != SQLITE_OK) goto fail;
    if (sqlite3_bind_int(stmt, 3, file.count) != SQLITE_OK) goto fail;
    if (sqlite3_bind_double(stmt, 4, file.score) != SQLITE_OK) goto fail;
    if (update) {
        if (sqlite3_bind_text(stmt, 5, file.path, -1, NULL) != SQLITE_OK) goto fail;
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) goto fail;

    sqlite3_finalize(stmt);
    sqlite3_close(d->conn);

    return 0;

    fail:
        sqlite3_finalize(stmt);
        sqlite3_close(d->conn);
        return 1;
}

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

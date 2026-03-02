#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <sqlite3.h>
#include <string.h>
#include <stdbool.h>
#include "db/db.h"


static const char *schema =
#include "db/schema.sql"
;


static double _calculate_score(int atime, int count);


int db_init(const char *db_path) {
    // Open a connection.
    sqlite3 *conn = NULL;
    if (sqlite3_open(db_path, &conn) != SQLITE_OK) return 1;

    // Use the schema to initialize the database.
    if (sqlite3_exec(conn, schema, NULL, NULL, NULL) != SQLITE_OK) {
        sqlite3_close(conn);
        return 2;
    }

    sqlite3_close(conn);
    return 0;
}


// Return files based on score. Highest first.
int db_get_top(const char *db_path, int limit, char ***paths, int *count) {
    // Open a database connection.
    sqlite3 *conn = NULL;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_open(db_path, &conn) != SQLITE_OK) return 1;

    // Prepare and bind statement.
    const char *sql =
        " SELECT path FROM files "
        " ORDER BY score DESC "
        " LIMIT ? ; "
    ;
    if (sqlite3_prepare_v2(conn, sql, -1, &stmt, NULL) != SQLITE_OK) goto fail;
    if (sqlite3_bind_int(stmt, 1, limit) != SQLITE_OK) goto fail;

    // Execute statement and fill the array.
    *paths = calloc(limit, sizeof **paths);
    for (*count = 0; sqlite3_step(stmt) == SQLITE_ROW; (*count)++) {
        // TODO: Replace unsafe strcpy.
        (*paths)[*count] = calloc(2048, sizeof (char));
        strcpy((*paths)[*count], (char *)sqlite3_column_text(stmt, 0));
    }

    sqlite3_finalize(stmt);
    sqlite3_close(conn);
    return 0;

    fail:
        sqlite3_finalize(stmt);
        sqlite3_close(conn);
        return 1;

}

int db_update_file(const char *db_path, const char *path, int atime) {
    // Open a database connection.
    sqlite3 *conn = NULL;
    sqlite3_stmt *stmt = NULL;
    if (sqlite3_open(db_path, &conn) != SQLITE_OK) return 1;

    // Prepare and bind statement.
    const char *sql_count = " SELECT count FROM files WHERE path = ?;";
    if (sqlite3_prepare_v2(conn, sql_count, -1, &stmt, NULL) != SQLITE_OK) goto fail;
    if (sqlite3_bind_text(stmt, 1, path, -1, NULL) != SQLITE_OK) goto fail;

    // Read the count if it exists, otherwise it's the first time we've seen it.
    int count = 0;
    int rc = sqlite3_step(stmt);
    switch (rc) {
        case SQLITE_ROW:
            count = sqlite3_column_int64(stmt, 1) + 1;
            break;
        case SQLITE_DONE:
            count = 1;
            break;
        default:
            goto fail;
    }
    sqlite3_finalize(stmt);

    // Calculate the score and insert/update the row.
    double score = _calculate_score(atime, count);
    const char *sql =
        " INSERT INTO files (path, atime, count, score) "
        " VALUES (?, ?, 1, ?) "
        " ON CONFLICT(path) DO UPDATE SET "
        "     atime = excluded.atime,     "
        "     count = files.count + 1,    "
        "     score = excluded.score;     ";
    if (sqlite3_prepare_v2(conn, sql, -1, &stmt, NULL) != SQLITE_OK) goto fail;
    if (sqlite3_bind_text(stmt, 1, path, -1, NULL) != SQLITE_OK) goto fail;
    if (sqlite3_bind_int(stmt, 2, atime) != SQLITE_OK) goto fail;
    if (sqlite3_bind_double(stmt, 3, score) != SQLITE_OK) goto fail;
    if (sqlite3_step(stmt) != SQLITE_DONE) goto fail;

    sqlite3_finalize(stmt);
    sqlite3_close(conn);
    return 0;

    fail:
        sqlite3_finalize(stmt);
        sqlite3_close(conn);
        return 1;
}


// #### Private Methods ####

double _calculate_score(int atime, int count) {
    time_t now = time(NULL);
    double dt = (double)(now - atime);
    double lambda = -0.00001;
    return count * exp(lambda * dt);
}

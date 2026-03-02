#ifndef DB_H
#define DB_H

#include <sqlite3.h>

typedef struct db db_t;

typedef struct {
    char *path;
    int atime;
    int count;
    double score;
} db_file_t;

db_t *db_init(const char *path);
int db_update_file(db_t *db, const char *path, int atime);
int db_get_top(db_t *db, int limit, db_file_t **results, int *count);
void db_free_results(db_file_t *files, int count);

#endif

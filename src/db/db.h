#ifndef DB_H
#define DB_H

#include <sqlite3.h>

typedef struct db db_t;

db_t *db_init(const char *path);
int db_update_file(db_t *db, const char *path, int atime);
int db_get_top(db_t *db, int limit, char ***paths, int *count);

#endif

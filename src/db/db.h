#ifndef DB_H
#define DB_H

#include <sqlite3.h>

typedef struct db db_t;

int db_init(const char *db_path);
int db_update_file(const char *db_path, const char *path, int atime);
int db_get_top(const char *db_path, int limit, char ***paths, int *count);

#endif

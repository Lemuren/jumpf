#ifndef DB_H
#define DB_H

int db_init(const char *db_path);
int db_update_file(const char *db_path, const char *path, int atime);
int db_get_top(const char *db_path, int limit, char ***paths, int *count);

#endif

"CREATE TABLE files ("
"    path        TEXT PRIMARY KEY,"
"    atime       INTEGER NOT NULL,"
"    count       INTEGER NOT NULL,"
"    score       REAL NOT NULL"
");"

"CREATE INDEX idx_score ON files(score DESC);"

CREATE TABLE files (
    path        TEXT PRIMARY KEY,   -- absolute, normalized path
    atime       INTEGER NOT NULL,   -- last access time (unix timestamp)
    count       INTEGER NOT NULL,   -- number of accesses
    score       REAL NOT NULL       -- precomputed score
);

CREATE INDEX idx_score ON files(score DESC);

/* ui/src/storage.c */
#include "storage.h"
#include "stealth_ui.h"
// Uncomment when SQLCipher is built via buildroot
// #include <sqlcipher/sqlite3.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

#define DB_PATH "/data/stealth.db"

// static sqlite3 *g_db = NULL;

int storage_open(const uint8_t *key32) {
    fprintf(stderr, "[storage] Opening SQLCipher DB (stubbed)\n");
    /*
    if (sqlite3_open(DB_PATH, &g_db) != SQLITE_OK) {
        return -1;
    }

    // Unlock with the 32-byte key derived from PIN via Argon2id
    if (sqlite3_key(g_db, key32, 32) != SQLITE_OK) {
        sqlite3_close(g_db);
        g_db = NULL;
        return -1;
    }

    // Verify the key worked by running a test query
    char *err = NULL;
    if (sqlite3_exec(g_db, "SELECT count(*) FROM sqlite_master;",
                     NULL, NULL, &err) != SQLITE_OK) {
        sqlite3_free(err);
        sqlite3_close(g_db);
        g_db = NULL;
        return -1;   // Wrong PIN / corrupted DB
    }

    // Create schema on first boot
    const char *schema =
        "CREATE TABLE IF NOT EXISTS contacts ("
        "  id      INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  name    TEXT NOT NULL,"
        "  number  TEXT NOT NULL UNIQUE"
        ");"
        "CREATE TABLE IF NOT EXISTS messages ("
        "  id        INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  number    TEXT NOT NULL,"
        "  body      TEXT NOT NULL,"
        "  direction INTEGER NOT NULL,"
        "  timestamp INTEGER NOT NULL"
        ");";

    sqlite3_exec(g_db, schema, NULL, NULL, NULL);
    */
    (void)key32;
    return 0;
}

void storage_close(void) {
    /*
    if (g_db) {
        sqlite3_close(g_db);
        g_db = NULL;
    }
    */
}

int storage_contact_add(const char *name, const char *number) {
    /*
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db,
        "INSERT OR IGNORE INTO contacts (name, number) VALUES (?, ?);",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, name,   -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, number, -1, SQLITE_STATIC);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
    */
    (void)name; (void)number;
    return 0;
}

int storage_contact_list(Contact *out, int max_count) {
    /*
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db,
        "SELECT id, name, number FROM contacts ORDER BY name;",
        -1, &stmt, NULL);
    int n = 0;
    while (n < max_count && sqlite3_step(stmt) == SQLITE_ROW) {
        out[n].id = sqlite3_column_int(stmt, 0);
        strncpy(out[n].name,   (const char*)sqlite3_column_text(stmt, 1), 63);
        strncpy(out[n].number, (const char*)sqlite3_column_text(stmt, 2), 31);
        n++;
    }
    sqlite3_finalize(stmt);
    return n;
    */
    (void)out; (void)max_count;
    return 0;
}

int storage_message_add(const char *number,
                         const char *body, int direction) {
    /*
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db,
        "INSERT INTO messages (number, body, direction, timestamp) "
        "VALUES (?, ?, ?, ?);",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, number, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, body,   -1, SQLITE_STATIC);
    sqlite3_bind_int (stmt, 3, direction);
    sqlite3_bind_int64(stmt, 4, (long)time(NULL));
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return (rc == SQLITE_DONE) ? 0 : -1;
    */
    (void)number; (void)body; (void)direction;
    return 0;
}

int storage_message_list(const char *number,
                          Message *out, int max_count) {
    /*
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db,
        "SELECT id, number, body, direction, timestamp "
        "FROM messages WHERE number=? ORDER BY timestamp DESC;",
        -1, &stmt, NULL);
    sqlite3_bind_text(stmt, 1, number, -1, SQLITE_STATIC);
    int n = 0;
    while (n < max_count && sqlite3_step(stmt) == SQLITE_ROW) {
        out[n].id        = sqlite3_column_int(stmt, 0);
        strncpy(out[n].number, (const char*)sqlite3_column_text(stmt, 1), 31);
        strncpy(out[n].body,   (const char*)sqlite3_column_text(stmt, 2), 255);
        out[n].direction  = sqlite3_column_int(stmt, 3);
        out[n].timestamp  = sqlite3_column_int64(stmt, 4);
        n++;
    }
    sqlite3_finalize(stmt);
    return n;
    */
    (void)number; (void)out; (void)max_count;
    return 0;
}

int storage_thread_list(Message *out, int max_count) {
    /*
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(g_db,
        "SELECT id, number, body, direction, timestamp "
        "FROM messages GROUP BY number ORDER BY timestamp DESC;",
        -1, &stmt, NULL);
    int n = 0;
    while (n < max_count && sqlite3_step(stmt) == SQLITE_ROW) {
        out[n].id        = sqlite3_column_int(stmt, 0);
        strncpy(out[n].number, (const char*)sqlite3_column_text(stmt, 1), 31);
        strncpy(out[n].body,   (const char*)sqlite3_column_text(stmt, 2), 255);
        out[n].direction  = sqlite3_column_int(stmt, 3);
        out[n].timestamp  = sqlite3_column_int64(stmt, 4);
        n++;
    }
    sqlite3_finalize(stmt);
    return n;
    */
    (void)out; (void)max_count;
    return 0;
}

int storage_message_delete_all(const char *number) {
    (void)number;
    return 0;
}

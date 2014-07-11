/* SQLite integration for GNU Emacs.
   This is free and unencumbered software released into the public domain. */
/*
  The purpose of this extension is to provide a high-quality, ACID
  database to Emacs extensions. No new primitive types are
  introduced. Instead, these functions return and operate on integer
  handles, which are indexes into internal pointer arrays. To make the
  Emacs Lisp interface nicer, these handles are wrapped in cl-lib
  structs on the Emacs Lisp side (sqlite3.el). */
#include <config.h>
#include "sqlite.h"

#ifdef HAVE_SQLITE3
#include <sqlite3.h>
#include "lisp.h"

/* Wrapper struct for sqlite3 database connections. It allows a
   per-connection array of statement objects. */
struct db {
  sqlite3 *db;
  sqlite3_stmt **stmts;
  int nstmts;
};

/* Returns the next available statement handle in DB. */
static int
next_stmt(struct db *db)
{
  for (int i = 0; i < db->nstmts; i++) {
    if (db->stmts[i] == NULL) {
      return i;
    }
  }
  db->nstmts *= 2;
  db->stmts = xrealloc(db->stmts, db->nstmts * sizeof(sqlite3_stmt *));
  for (int i = db->nstmts / 2; i < db->nstmts; i++) {
    db->stmts[i] = NULL;
  }
  return db->nstmts / 2;
}

/* List of all open databases. Database handles index into this
   array. */
static struct db *databases;
static int ndatabases;

/* Return the next available database handle. */
static int
next_database()
{
  for (int i = 0; i < ndatabases; i++) {
    if (databases[i].db == NULL) {
      return i;
    }
  }
  ndatabases *= 2;
  databases = xrealloc(databases, ndatabases * sizeof(struct db));
  for (int i = ndatabases / 2; i < ndatabases; i++) {
    databases[i].db = NULL;
  }
  return ndatabases / 2;
}

static void
db_init(struct db *db)
{
  db->nstmts = 8;
  size_t size = db->nstmts * sizeof(sqlite3_stmt *);
  db->stmts = xmalloc(size);
  memset(db->stmts, 0, size);
}

static void
db_free(struct db *db)
{
  if (db->db != NULL) {
    db->db = NULL;
    free(db->stmts);
  }
}

#define CHECK_HANDLE(i)                                        \
  do {                                                         \
    CHECK_RANGED_INTEGER(i, 0, ndatabases - 1);                \
    if (databases[XINT(i)].db == NULL)                         \
      error ("Invalid SQLite database");                       \
  } while (false);

#define CHECK_STMT(db, stmti)                                  \
  do {                                                         \
    CHECK_RANGED_INTEGER(stmti, 0, db->nstmts - 1);            \
    if (db->stmts[XINT(stmti)] == NULL)                        \
      error ("Invalid SQLite statement");                      \
  } while (false);

DEFUN ("sqlite3-open-1", Fsqlite3_open_1, Ssqlite3_open_1, 1, 1, 0,
       doc: /* Opens the database at FILENAME, creating it if necessary.
Returns an integer handle representing the connection. It can be
closed later with `sqlite3-close-1'.

This is an internal function and should not be used directly. */)
  (Lisp_Object filename)
{
  CHECK_STRING (filename);
  char *c_filename = SDATA (filename);
  int dbi = next_database();
  if (sqlite3_open(c_filename, &databases[dbi].db) != SQLITE_OK) {
    sqlite3 *tmp = databases[dbi].db;
    databases[dbi].db = NULL;
    error ("SQLite error: %s", sqlite3_errmsg (tmp));
  } else {
    db_init(&databases[dbi]);
    return make_number(dbi);
  }
}

DEFUN ("sqlite3-close-1", Fsqlite3_close_1, Ssqlite3_close_1, 1, 1, 0,
       doc: /* Close database connection to HANDLE.
This is an internal function and should not be used directly. */)
  (Lisp_Object handle)
{
  CHECK_RANGED_INTEGER(handle, 0, ndatabases - 1); // NULL allowed
  int dbi = XINT(handle);
  if (sqlite3_close(databases[dbi].db) != SQLITE_OK) {
    error ("SQLite error: %s", sqlite3_errmsg (databases[dbi].db));
  } else {
    db_free(&databases[dbi]);
  }
  return Qnil;
}

DEFUN ("sqlite3-prepare-1", Fsqlite3_prepare_1, Ssqlite3_prepare_1, 2, 2, 0,
       doc: /* Create a prepared statement from a SQL expression.
This is an internal function and should not be used directly. */)
  (Lisp_Object handle, Lisp_Object sql)
{
  CHECK_HANDLE (handle);
  CHECK_STRING (sql);
  int dbi = XINT(handle);
  char *c_sql = SDATA (sql);
  int bytes = SBYTES(sql);
  int stmti = next_stmt(&databases[dbi]);
  sqlite3 *db = databases[dbi].db;
  sqlite3_stmt **stmt = &databases[dbi].stmts[stmti];
  if (sqlite3_prepare_v2(db, c_sql, bytes, stmt, NULL) != SQLITE_OK) {
    error ("SQLite error: %s", sqlite3_errmsg (databases[dbi].db));
  }
  return make_number (stmti);
}

DEFUN ("sqlite3-finalize-1", Fsqlite3_finalize_1, Ssqlite3_finalize_1, 2, 2, 0,
       doc: /* Free a prepared statement.
This is an internal function and should not be used directly. */)
  (Lisp_Object handle, Lisp_Object stmt)
{
  CHECK_HANDLE (handle);
  struct db *db = &databases[XINT(handle)];
  CHECK_STMT (db, stmt);
  sqlite3_stmt *c_stmt = db->stmts[XINT(stmt)];
  db->stmts[XINT(stmt)] = NULL; // freed regardless of error
  if (sqlite3_finalize(c_stmt) != SQLITE_OK) {
    error ("SQLite error: %s", sqlite3_errmsg (db->db));
  }
  return Qnil;
}

DEFUN ("sqlite3-reset-1", Fsqlite3_reset_1, Ssqlite3_reset_1, 2, 2, 0,
       doc: /* Reset a prepared statement.
This is an internal function and should not be used directly. */)
  (Lisp_Object handle, Lisp_Object stmt)
{
  CHECK_HANDLE (handle);
  struct db *db = &databases[XINT(handle)];
  CHECK_STMT (db, stmt);
  sqlite3_stmt *c_stmt = db->stmts[XINT(stmt)];
  db->stmts[XINT(stmt)] = NULL; // freed regardless of error
  if (sqlite3_reset(c_stmt) != SQLITE_OK) {
    error ("SQLite error: %s", sqlite3_errmsg (db->db));
  }
  return Qnil;
}

DEFUN ("sqlite3-bind-1", Fsqlite3_bind_1, Ssqlite3_bind_1, 4, 4, 0,
       doc: /* Bind VALUE in a prepared statement.
This is an internal function and should not be used directly. */)
  (Lisp_Object handle, Lisp_Object stmt, Lisp_Object n, Lisp_Object value)
{
  CHECK_HANDLE (handle);
  struct db *db = &databases[XINT(handle)];
  CHECK_STMT (db, stmt);
  sqlite3_stmt *c_stmt = db->stmts[XINT(stmt)];
  CHECK_NUMBER (n);
  int ret;
  if (INTEGERP (value)) {
    ret = sqlite3_bind_int64(c_stmt, XINT(n), XINT(value));
  } else if (FLOATP (value)) {
    ret = sqlite3_bind_double(c_stmt, XINT(n), extract_float (value));
  } else if (EQ (value, Qnil)) {
    ret = sqlite3_bind_null(c_stmt, XINT(n));
  } else if (STRING_MULTIBYTE (value)) {
    ret = sqlite3_bind_text(c_stmt, XINT(n), SDATA (value), SBYTES (value),
                            SQLITE_TRANSIENT);
  } else if (STRINGP (value)) { // unibyte
    ret = sqlite3_bind_blob(c_stmt, XINT(n), SDATA (value), SBYTES (value),
                            SQLITE_TRANSIENT);
  } else {
    error ("Invalid SQLite value type");
  }
  if (ret != SQLITE_OK) {
    error ("SQLite error: %s", sqlite3_errmsg (db->db));
  }
  return value;
}

DEFUN ("sqlite3-column-1", Fsqlite3_column_1, Ssqlite3_column_1,
       3, 3, 0,
       doc: /* Return the value from the current row in column N.
This is an internal function and should not be used directly. */)
  (Lisp_Object handle, Lisp_Object stmt, Lisp_Object n)
{
  CHECK_HANDLE (handle);
  struct db *db = &databases[XINT(handle)];
  CHECK_STMT (db, stmt);
  sqlite3_stmt *c_stmt = db->stmts[XINT(stmt)];
  CHECK_NUMBER(n);
  int i = XINT(n);
  int type = sqlite3_column_type(c_stmt, i);
  if (type == SQLITE_INTEGER) {
    return make_number (sqlite3_column_int64(c_stmt, i));
  } else if (type == SQLITE_FLOAT) {
    return make_float (sqlite3_column_double(c_stmt, i));
  } else if (type == SQLITE_NULL) {
    return Qnil;
  } else if (type == SQLITE_TEXT) {
    const char *text = sqlite3_column_text (c_stmt, i);
    int bytes = sqlite3_column_bytes(c_stmt, i);
    return make_string(text, bytes);
  } else if (type == SQLITE_BLOB) {
    const void *blob = sqlite3_column_blob (c_stmt, i);
    int bytes = sqlite3_column_bytes(c_stmt, i);
    return make_unibyte_string(blob, bytes);
  }
}

DEFUN ("sqlite3-column-count-1",
       Fsqlite3_column_count_1, Ssqlite3_column_count_1,
       2, 2, 0,
       doc: /* Return the number of result columns for a prepared statement.
This is an internal function and should not be used directly. */)
  (Lisp_Object handle, Lisp_Object stmt)
{
  CHECK_HANDLE (handle);
  struct db *db = &databases[XINT(handle)];
  CHECK_STMT (db, stmt);
  sqlite3_stmt *c_stmt = db->stmts[XINT(stmt)];
  return make_number(sqlite3_column_count(c_stmt));
}

DEFUN ("sqlite3-step-1", Fsqlite3_step_1, Ssqlite3_step_1,
       2, 2, 0,
       doc: /* Step a statement forward, returning t when there are more rows.
This is an internal function and should not be used directly. */)
  (Lisp_Object handle, Lisp_Object stmt)
{
  CHECK_HANDLE (handle);
  struct db *db = &databases[XINT(handle)];
  CHECK_STMT (db, stmt);
  sqlite3_stmt *c_stmt = db->stmts[XINT(stmt)];
  int ret = sqlite3_step(c_stmt);
  if (ret == SQLITE_ROW) {
    return Qt;
  } else if (ret == SQLITE_DONE) {
    return Qnil;
  } else {
    error ("SQLite error: %s", sqlite3_errmsg (db->db));
  }
}

DEFUN ("sqlite3-available-p", Fsqlite3_available_p, Ssqlite3_available_p,
       0, 0, 0,
       doc: /* Returns t if SQLite functions are available. */)
  ()
{
  return Qt;
}

void
syms_of_sqlite3 (void)
{
  defsubr (&Ssqlite3_open_1);
  defsubr (&Ssqlite3_close_1);
  defsubr (&Ssqlite3_prepare_1);
  defsubr (&Ssqlite3_finalize_1);
  defsubr (&Ssqlite3_reset_1);
  defsubr (&Ssqlite3_bind_1);
  defsubr (&Ssqlite3_column_1);
  defsubr (&Ssqlite3_column_count_1);
  defsubr (&Ssqlite3_step_1);
  defsubr (&Ssqlite3_available_p);

  ndatabases = 16;
  size_t size = ndatabases * sizeof(struct db);
  databases = xmalloc(size);
  memset(databases, 0, size);
}

#else /* !HAVE_SQLITE3 */

DEFUN ("sqlite3-available-p", Fsqlite3_available_p, Ssqlite3_available_p,
       0, 0, 0,
       doc: /* Returns t if SQLite functions are available. */)
  ()
{
  return Qnil;
}

void
syms_of_sqlite3 ()
{
  defsubr (&Ssqlite3_available_p);
}

#endif /* HAVE_SQLITE3 */

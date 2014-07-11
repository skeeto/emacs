;;; sqlite3.el --- SQLite integration -*- lexical-binding: t; -*-

;; This is free and unencumbered software released into the public domain.

;; This file is part of GNU Emacs.

;; Author: Christopher Wellons <wellons@nullprogram.com>

;;; Commentary:

;;; Code:

(require 'cl-lib)

(cl-defstruct (sqlite3-db (:constructor sqlite3-db--create) (:copier nil))
  "A SQLite database connection."
  handle file stmts)

(cl-defstruct (sqlite3-stmt (:constructor sqlite3-stmt--create) (:copier nil))
  "A SQLite prepared statement."
  handle db sql)

(defun sqlite3-stmt-db-handle (stmt)
  "Return the database handle for STMT."
  (sqlite3-db-handle (sqlite3-stmt-db stmt)))

(defun sqlite3-open (file)
  "Open the SQLite database stored in FILE, creating the file if needed."
  (sqlite3-db--create :handle (sqlite3-open-1 file) :file file))

(defun sqlite3-close (db)
  "Close SQLite database connection DB."
  (mapc #'sqlite3-finalize (sqlite3-db-stmts db))
  (sqlite3-close-1 (sqlite3-db-handle db)))

(defun sqlite3-prepare (db sql)
  "Compile and return a prepared statement for SQL."
  (let* ((handle (sqlite3-prepare-1 (sqlite3-db-handle db) sql))
         (stmt (sqlite3-stmt--create :handle handle :db db :sql sql)))
    (prog1 stmt
      (push stmt (sqlite3-db-stmts db)))))

(defun sqlite3-finalize (stmt)
  "Destroy the prepared statement STMT."
  (let ((db (sqlite3-stmt-db stmt)))
    (prog1 nil
      (sqlite3-finalize-1 (sqlite3-db-handle db) (sqlite3-stmt-handle stmt))
      (setf (sqlite3-db-stmts db)
            (cl-delete (sqlite3-stmt-handle stmt) (sqlite3-db-stmts db)
                       :key #'sqlite3-stmt-handle)))))

(defun sqlite3-bind (stmt n value)
  "Bind VALUE to 1-indexed position N in prepared statement STMT.

Value must be an integer, float, nil, or a string. Multibyte
strings map to a SQLite STRING value, unibyte strings map to a
SQLite BLOB value, and nil maps to SQLite's NULL."
  (sqlite3-bind-1 (sqlite3-stmt-db-handle stmt)
                  (sqlite3-stmt-handle stmt) n value))

(defun sqlite3-bind-values (stmt &rest values)
  "Bind VALUES to parallel positions in prepared statement STMT.
This is a convenience wrapper for `sqlite3-bind'."
  (cl-loop for i upfrom 1
           for value in values
           do (sqlite3-bind stmt i value)))

(defun sqlite3-column (stmt n)
  "Return the value for 0-indexed column N in prepared statement N.

NULL will be returned as nil. TEXT and BLOB values will be
returned as a unibyte and multibyte strings."
  (sqlite3-column-1 (sqlite3-stmt-db-handle stmt) (sqlite3-stmt-handle stmt) n))

(defun sqlite3-column-count (stmt)
  "Return the number of result columns for prepared statement STMT."
  (sqlite3-column-count-1 (sqlite3-stmt-db-handle stmt)
                          (sqlite3-stmt-handle stmt)))

(defun sqlite3-step (stmt)
  "Step prepared statement STMT, returning t when there are more rows."
  (sqlite3-step-1 (sqlite3-stmt-db-handle stmt)
                  (sqlite3-stmt-handle stmt)))

(defun sqlite3-exec (db sql &rest values)
  "Compile, execute, and finalize SQL expression returning the result rows."
  (let* ((stmt (sqlite3-prepare db sql))
         (count (sqlite3-column-count stmt)))
    (unwind-protect
        (progn
          (apply #'sqlite3-bind-values stmt values)
          (cl-loop while (sqlite3-step stmt)
                   collect (cl-loop for i from 0 below count
                                    collect (sqlite3-column stmt i))))
      (sqlite3-finalize stmt))))

(provide 'sqlite3)

;;; sqlite3.el ends here

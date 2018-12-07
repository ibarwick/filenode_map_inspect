filenode_map_inspect
====================

A simple extension providing functionality to check and examine
the `pg_filenode.map` files present in PostgreSQL.

This is an experimental extension for diagnostic purposes only.
USE AT YOUR OWN RISK!

Requirements
------------

PostgreSQL 9.0 or later.

Functions
---------

 - `filenode_map_check()`
  - lists all files

 - filenode_map_list():
  - lists oid, relfilenode, relname of current database's `pg_filenode.map` file

See also
--------

 - source code: `src/backend/utils/cache/relmapper.c`


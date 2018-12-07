filenode_map_inspect
====================

A simple extension providing functionality to check and examine
the `pg_filenode.map` files present in PostgreSQL.

This is an experimental extension for educational and diagnostic purposes only.
USE AT YOUR OWN RISK!

Requirements
------------

This extension will compile with PostgreSQL 9.6 or later (though
it should be possible to adapt it easily to earlier versions back
to 9.0, where `pg_filenode.map` was introduced).

Functions
---------

 - `filenode_map_check()`: lists the location of all `pg_filenode.map` files
      present in the database cluster, and verifies their status.

 - `filenode_map_list()`:  lists name, oid and filenode of the current database's
      `pg_filenode.map` file

Examples
--------

Output of `filenode_map_check()`:

	postgres=# SELECT * FROM filenode_map_check();
	WARNING:  could not read file "base/32774/pg_filenode.map": read 256 of 512
	 database  |            file            |       state
	-----------+----------------------------+-------------------
	 postgres  | base/12630/pg_filenode.map | OK
	 template1 | base/1/pg_filenode.map     | OK
	 template0 | base/12629/pg_filenode.map | OK
	 foo       | base/32774/pg_filenode.map | FILE_ACCESS_ERROR
	(4 rows)

Here we see the filenode map for database `foo` is incomplete. We could establish
exactly the same result by attempting to connect to `foo`. The advantage of this
function is that (assuming at least one database can be connected to) it will
present an overview of the state of all database `pg_filenode.map` files.

Output of `filenode_map_list()`:

	repmgr=# SELECT * FROM filenode_map_list();
				  relname              | oid  | filenode
	-----------------------------------+------+----------
	 pg_class                          | 1259 |     1259
	 pg_attribute                      | 1249 |     1249
	 pg_proc                           | 1255 |     1255
	 pg_type                           | 1247 |     1247
	 pg_toast_1255                     | 2836 |     2836
	 pg_toast_1255_index               | 2837 |     2837
	 pg_attribute_relid_attnam_index   | 2658 |     2658
	 pg_attribute_relid_attnum_index   | 2659 |     2659
	 pg_class_oid_index                | 2662 |     2662
	 pg_class_relname_nsp_index        | 2663 |     2663
	 pg_class_tblspc_relfilenode_index | 3455 |     3455
	 pg_proc_oid_index                 | 2690 |     2690
	 pg_proc_proname_args_nsp_index    | 2691 |     2691
	 pg_type_oid_index                 | 2703 |     2703
	 pg_type_typname_nsp_index         | 2704 |     2704
	(15 rows)

Note that currently this cannot display the contents of the global `pg_filenode.map`
file.

Notes
-----

The practical utility of this extension is debatable.

In practice, cases of `pg_filenode.map` corruption appear to be seldom
and usually self-inflicted.

See also
--------

 - source code: `src/backend/utils/cache/relmapper.c`
 - [Decoding pg_filenode.map files with pg_filenodemapdata](http://pgeoghegan.blogspot.com/2018/03/decoding-pgfilenodemapdata.html)

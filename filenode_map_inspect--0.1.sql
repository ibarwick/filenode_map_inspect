/* filenode_map_inspect/filenode_map_inspect--0.1.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION filenode_map_inspect" to load this file.\quit


CREATE OR REPLACE FUNCTION filenode_map_check(
	OUT database TEXT,
    OUT file TEXT,
	OUT state TEXT
  )
  RETURNS SETOF record
  AS 'MODULE_PATHNAME'
  LANGUAGE C STRICT VOLATILE PARALLEL UNSAFE;

CREATE OR REPLACE FUNCTION filenode_map_list(
    OUT relname TEXT,
    OUT oid OID,
    OUT filenode OID
  )
  RETURNS SETOF record
  AS 'MODULE_PATHNAME'
  LANGUAGE C STRICT VOLATILE PARALLEL UNSAFE;

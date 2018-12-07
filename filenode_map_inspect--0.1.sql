/* filenode_map_inspect/filenode_map_inspect--0.1.sql */

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION filenode_map_inspect" to load this file.\quit


CREATE OR REPLACE FUNCTION filenode_map_check(
    OUT file TEXT
  )
  RETURNS SETOF record
  AS 'MODULE_PATHNAME'
  LANGUAGE C STRICT VOLATILE PARALLEL UNSAFE;

CREATE OR REPLACE FUNCTION filenode_map_list(
    OUT oid OID,
    out filenode OID,
    out relname TEXT
  )
  RETURNS SETOF record
  AS 'MODULE_PATHNAME'
  LANGUAGE C STRICT VOLATILE PARALLEL UNSAFE;

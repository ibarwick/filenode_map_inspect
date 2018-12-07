/*
 * filenode_map_inspect.c
 *
 */

#include "postgres.h"

#include <unistd.h>


#include "fmgr.h"
#include "funcapi.h"
#include "miscadmin.h"

#include "access/heapam.h"
#include "access/xact.h"
#include "catalog/pg_database.h"
#include "storage/lwlock.h"
#include "utils/builtins.h"
#include "utils/rel.h"
#include "utils/relmapper.h"

#if (PG_VERSION_NUM >= 90300)
#include "access/htup_details.h"
#endif


PG_MODULE_MAGIC;

/* struct to store list of databases */
typedef struct db_list
{
	Oid			datid;
	char	   *datname;
} db_list;

typedef enum
{
	file_ok,
	file_access_error,
	file_crc_error,
	file_invalid_error
} FilenodeMapStatus;

/*
 * Following definitions from src/backend/utils/cache/relmapper.c
 */

#define RELMAPPER_FILENAME		"pg_filenode.map"

/* As of PostgreSQL 12, this value has never changed */
#define RELMAPPER_FILEMAGIC		0x592717	/* version ID value */

#define MAX_MAPPINGS			62	/* 62 * 8 + 16 = 512 */

typedef struct RelMapping
{
	Oid			mapoid;			/* OID of a catalog */
	Oid			mapfilenode;	/* its filenode number */
} RelMapping;

typedef struct RelMapFile
{
	int32		magic;			/* always RELMAPPER_FILEMAGIC */
	int32		num_mappings;	/* number of valid RelMapping entries */
	RelMapping	mappings[MAX_MAPPINGS];
	pg_crc32c	crc;			/* CRC of all above */
	int32		pad;			/* to make the struct size be 512 exactly */
} RelMapFile;


void _PG_init(void);

/* SQL functions */

Datum filenode_map_check(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(filenode_map_check);

Datum filenode_map_list(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(filenode_map_list);

/* internal functions */

static FilenodeMapStatus check_relmap_file(const char *file);
static bool read_relmap_file(const char *file, char **buffer);

/*
 * Entrypoint of this module.
 */
void
_PG_init(void)
{
	/* Do something here */
}


/**
 * filenode_map_check()
 *
 *
 */

Datum filenode_map_check(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;

	List	   *our_dblist = NIL;
	ListCell   *cell;

	Relation	rel;
	HeapTuple	tup;
	HeapScanDesc scan;
	MemoryContext per_query_ctx;
	MemoryContext resultcxt;
	MemoryContext oldcontext;
	TupleDesc	tupdesc;
	Tuplestorestate *tupstore;

	/* check to see if caller supports this function returning a tuplestore */

	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));

	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required, but it is not " \
						"allowed in this context")));

	/* This is the context that we will allocate our output data in */
	resultcxt = CurrentMemoryContext;

	/*
	 * get OIDs of each database
	 */

	rel = heap_open(DatabaseRelationId, AccessShareLock);
	scan = heap_beginscan_catalog(rel, 0, NULL);

	while (HeapTupleIsValid(tup = heap_getnext(scan, ForwardScanDirection)))
	{
		Form_pg_database pgdatabase = (Form_pg_database) GETSTRUCT(tup);
		db_list *dblist_entry = NULL;
		MemoryContext oldcxt;

		oldcxt = MemoryContextSwitchTo(resultcxt);

		dblist_entry = (db_list *) palloc0(sizeof(db_list));

#if (PG_VERSION_NUM >= 120000)
		dblist_entry->datid = pgdatabase->oid;
#else
		dblist_entry->datid = HeapTupleGetOid(tup);
#endif
		dblist_entry->datname = pstrdup(NameStr(pgdatabase->datname));

		our_dblist = lappend(our_dblist, dblist_entry);

		MemoryContextSwitchTo(oldcxt);
	}

	heap_endscan(scan);
	heap_close(rel, AccessShareLock);

	/* Switch into long-lived context to construct returned data structures */
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	/* Build a tuple descriptor for function's result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	/*
	 * scan database list and examine each file
	 */

	foreach(cell, our_dblist)
	{
		db_list	   *currdb = lfirst(cell);
		char		mapfilename[MAXPGPATH];
		FilenodeMapStatus relmap_file_status;

		Datum		values[3];
		bool		nulls[3];

		memset(values, 0, sizeof(values));
		memset(nulls, 0, sizeof(nulls));

		snprintf(mapfilename, sizeof(mapfilename), "base/%i/%s",
				 currdb->datid, RELMAPPER_FILENAME);

		values[0] = CStringGetTextDatum(currdb->datname);
		values[1] = CStringGetTextDatum(mapfilename);

		relmap_file_status = check_relmap_file(mapfilename);

		switch (relmap_file_status)
		{
			case file_ok:
				values[2] = CStringGetTextDatum("OK");
				break;
			case file_access_error:
				values[2] = CStringGetTextDatum("FILE_ACCESS_ERROR");
				break;
			case file_crc_error:
				values[2] = CStringGetTextDatum("FILE_CRC_ERROR");
				break;
			case file_invalid_error:
				values[2] = CStringGetTextDatum("FILE_FORMAT_ERROR");
				break;

			default:
				values[2] = CStringGetTextDatum("unknown FilenodeMapStatus");
				break;
		}

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
	}

	/* no more rows */
	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}

/**
 * filenode_map_list()
 *
 *
 */

Datum filenode_map_list(PG_FUNCTION_ARGS)
{
	RelMapFile *map = NULL;
	char		mapfilename[MAXPGPATH];
	int			i;

	snprintf(mapfilename, sizeof(mapfilename), "%s/%s",
				 DatabasePath, RELMAPPER_FILENAME);

	if (read_relmap_file(mapfilename, (char **)&map) == false)
		return file_access_error;

	elog(INFO, "num_mappings: %i", map->num_mappings);

	for (i = 0; i < map->num_mappings; i++)
	{
		Relation        rel;

		rel = relation_open(map->mappings[i].mapoid, AccessShareLock);

		elog(INFO, "oid: %u, filenode: %u; name: %s",
			 map->mappings[i].mapoid, map->mappings[i].mapfilenode, RelationGetRelationName(rel));

		relation_close(rel, AccessShareLock);
	}

	PG_RETURN_NULL();
}


/**
 * check_relmap_file()
 *
 * Note that in the corresponding backend function, FATAL will be
 * reported if a file error is encountered; however in the assumption
 * we're reading another database's file, we'll merely emit a warning.
 */

static FilenodeMapStatus
check_relmap_file(const char *mapfilename)
{
	RelMapFile *map = NULL;
	FilenodeMapStatus retval = file_ok;

	if (read_relmap_file(mapfilename, (char **)&map) == false)
		return file_access_error;

	/* check for correct magic number, etc */
	if (map->magic != RELMAPPER_FILEMAGIC ||
		map->num_mappings < 0 ||
		map->num_mappings > MAX_MAPPINGS)
	{
		ereport(WARNING,
				(errmsg("relation mapping file \"%s\" contains invalid data",
						mapfilename)));
		retval = file_invalid_error;
	}
	else
	{
		pg_crc32c	crc;

		/* verify the CRC */
		INIT_CRC32C(crc);
		COMP_CRC32C(crc, (char *) map, offsetof(RelMapFile, crc));
		FIN_CRC32C(crc);

		if (!EQ_CRC32C(crc, map->crc))
		{
			ereport(WARNING,
					(errmsg("relation mapping file \"%s\" contains incorrect checksum",
							mapfilename)));
			retval = file_crc_error;
		}
	}

	pfree(map);

	return retval;
}


static bool
read_relmap_file(const char *mapfilename, char **buffer)
{
	FILE	   *fp = NULL;
	int			r;

	*buffer = palloc0(sizeof(RelMapFile));

	fp = AllocateFile(mapfilename, PG_BINARY_R);

	if (fp == NULL)
	{
		ereport(WARNING,
				(errcode_for_file_access(),
				 errmsg("could not open file \"%s\": %m",
						mapfilename)));

		return false;
	}

	r = fread(*buffer, 1, sizeof(RelMapFile), fp);

	if (r != sizeof(RelMapFile))
	{
		if (r < 0)
			ereport(WARNING,
					(errcode_for_file_access(),
					 errmsg("could not read file \"%s\": %m", mapfilename)));
		else
			ereport(WARNING,
					(errcode(ERRCODE_DATA_CORRUPTED),
					 errmsg("could not read file \"%s\": read %d of %zu",
					 mapfilename, r, sizeof(RelMapFile))));

		FreeFile(fp);

		return false;
	}

	FreeFile(fp);

	return true;
}

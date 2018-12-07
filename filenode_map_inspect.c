/*
 * filenode_map_inspect.c
 *
 */

#include "postgres.h"
#include "fmgr.h"

PG_MODULE_MAGIC;

void _PG_init(void);

Datum filenode_map_check(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(filenode_map_check);

Datum filenode_map_list(PG_FUNCTION_ARGS);
PG_FUNCTION_INFO_V1(filenode_map_list);

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
	PG_RETURN_NULL();
}

/**
 * filenode_map_list()
 *
 *
 */

Datum filenode_map_list(PG_FUNCTION_ARGS)
{
	PG_RETURN_NULL();
}

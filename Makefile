# See:
#   http://www.postgresql.org/docs/current/interactive/extend-pgxs.html
# for all variables

EXTENSION   = filenode_map_inspect
EXTVERSION 	= 0.1

MODULES = $(EXTENSION)
# MODULE_big = shared library to build from object files listed in OBJS
# PROGRAM = executable program to build from object files listed in OBJS

OBJS = filenode_map_inspect.o

DATA = filenode_map_inspect--0.1.sql

TESTS        = $(wildcard test/sql/*.sql)
REGRESS      = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test --outputdir=test \
	--load-language=plpgsql --load-extension=$(EXTENSION)

PG_CONFIG = pg_config

PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

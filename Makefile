#http://blog.pgxn.org/post/4783001135/extension-makefiles pg makefiles

EXTENSION = pgexpanded
PG_CONFIG ?= pg_config
DATA = $(wildcard *--*.sql)
PGXS := $(shell $(PG_CONFIG) --pgxs)
MODULE_big = pgexpanded
OBJS = $(patsubst %.c,%.o,$(wildcard src/pgexpanded.c))
PG_CPPFLAGS = -O0

TESTS        = $(wildcard test/sql/*.sql)
REGRESS      = $(patsubst test/sql/%.sql,%,$(TESTS))
REGRESS_OPTS = --inputdir=test --load-language=plpgsql
include $(PGXS)

sql: pgexpanded--0.0.1.sql
	bash pgexpanded--0.0.1-gen.sql.sh

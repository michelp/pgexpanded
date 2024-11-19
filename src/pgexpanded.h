#ifndef PGEXPANDED_H
#define PGEXPANDED_H

#include "postgres.h"
#include "funcapi.h"
#include "utils/expandeddatum.h"

/* ID for debugging crosschecks */
#define exobj_MAGIC 689276813

#define LOGF() elog(DEBUG1, __func__)

/* Flattened representation of exobj, used to store to disk.

   The first 32 bits must the length of the data.  Actual flattened data
   is appended after this struct and cannot exceed 1GB.
*/
typedef struct pgexpanded_FlatExobj {
	int32 vl_len_;
} pgexpanded_FlatExobj;

/* Expanded representation of exobj.

   When loaded from storage, the flattened representation is used to
   build the exobj.  In this case, it's just a pointer to an integer.
*/
typedef struct pgexpanded_Exobj  {
	ExpandedObjectHeader hdr;
	int em_magic;
	Size flat_size;
	int64_t *value;
} pgexpanded_Exobj;

/* Callback function for freeing exobj arrays. */
static void
context_callback_exobj_free(void*);

/* Expanded Object Header "methods" for flattening for storage */
static Size
exobj_get_flat_size(ExpandedObjectHeader *eohptr);

static void
exobj_flatten_into(ExpandedObjectHeader *eohptr,
				   void *result, Size allocated_size);

static const ExpandedObjectMethods exobj_methods = {
	exobj_get_flat_size,
	exobj_flatten_into
};

/* Create a new exobj datum. */
pgexpanded_Exobj *
new_expanded_exobj(int64_t value,  MemoryContext parentcontext);

/* Helper function that either detoasts or expands. */
pgexpanded_Exobj *DatumGetExobj(Datum d);

/* Helper macro to detoast and expand exobjs arguments */
#define PGEXPANDED_GETARG_EXOBJ(n)  DatumGetExobj(PG_GETARG_DATUM(n))

/* Helper macro to return Expanded Object Header Pointer from exobj. */
#define PGEXPANDED_RETURN_EXOBJ(A) return EOHPGetRWDatum(&(A)->hdr)

/* Helper macro to compute flat exobj header size */
#define PGEXPANDED_EXOBJ_OVERHEAD() MAXALIGN(sizeof(pgexpanded_FlatExobj))

/* Helper macro to get pointer to beginning of exobj data. */
#define PGEXPANDED_EXOBJ_DATA(a) ((int64_t *)(((char *) (a)) + PGEXPANDED_EXOBJ_OVERHEAD()))

/* Help macro to cast generic Datum header pointer to expanded Exobj */
#define ExobjGetEOHP(d) (pgexpanded_Exobj *) DatumGetEOHP(d);

/* Public API functions */

PG_FUNCTION_INFO_V1(exobj);
PG_FUNCTION_INFO_V1(exobj_in);
PG_FUNCTION_INFO_V1(exobj_out);
PG_FUNCTION_INFO_V1(exobj_info);

void
_PG_init(void);

#endif /* PGEXPANDED_H */
/* Local Variables: */
/* mode: c */
/* c-file-style: "postgresql" */
/* End: */

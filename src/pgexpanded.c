#include "pgexpanded.h"
PG_MODULE_MAGIC;

/* Compute flattened size of storage needed for a exobj */
static Size
exobj_get_flat_size(ExpandedObjectHeader *eohptr) {
    pgexpanded_Exobj *A = (pgexpanded_Exobj*) eohptr;
    Size nbytes;

    LOGF();

    /* This is a sanity check that the object is initialized */
    Assert(A->em_magic == exobj_MAGIC);

    /* Use cached value if already computed */
    if (A->flat_size) {
        return A->flat_size;
    }

    // Add the overhead of the flat header to the size of the data
    // payload
    nbytes = PGEXPANDED_EXOBJ_OVERHEAD();
    nbytes += sizeof(uint64_t);

    /* Cache this value in the expanded object */
    A->flat_size = nbytes;
    return nbytes;
}

/* Flatten exobj into a pre-allocated result buffer that is
   allocated_size in bytes.  */
static void
exobj_flatten_into(ExpandedObjectHeader *eohptr,
                    void *result, Size allocated_size)  {
    void *data;

    /* Cast EOH pointer to expanded object, and result pointer to flat
       object */
    pgexpanded_Exobj *A = (pgexpanded_Exobj *) eohptr;
    pgexpanded_FlatExobj *flat = (pgexpanded_FlatExobj *) result;

    LOGF();

    /* Sanity check the object is valid */
    Assert(A->em_magic == exobj_MAGIC);
    Assert(allocated_size == A->flat_size);

    /* Zero out the whole allocated buffer */
    memset(flat, 0, allocated_size);

    /* Get the pointer to the start of the flattened data and copy the
       expanded value into it */
    data = PGEXPANDED_EXOBJ_DATA(flat);
	memcpy(data, A->value, sizeof(int64_t));

    /* Set the size of the varlena object */
    SET_VARSIZE(flat, allocated_size);
}

/* Expand a flat exobj in to an Expanded one, return as Postgres Datum. */
pgexpanded_Exobj *
new_expanded_exobj(int64_t value, MemoryContext parentcontext) {
  pgexpanded_Exobj *A;

  MemoryContext objcxt, oldcxt;
  MemoryContextCallback *ctxcb;

  LOGF();

  /* Create a new context that will hold the expanded object. */
  objcxt = AllocSetContextCreate(parentcontext,
                                 "expanded exobj",
                                 ALLOCSET_DEFAULT_SIZES);

  /* Allocate a new expanded exobj */
  A = (pgexpanded_Exobj*)MemoryContextAlloc(objcxt,
                                             sizeof(pgexpanded_Exobj));

  /* Initialize the ExpandedObjectHeader member with flattening
   * methods and the new object context */
  EOH_init_header(&A->hdr, &exobj_methods, objcxt);

  /* Used for debugging checks */
  A->em_magic = exobj_MAGIC;

  /* Switch to new object context */
  oldcxt = MemoryContextSwitchTo(objcxt);

  /* Get value from flat object and increment it */
  A->value = palloc(sizeof(int64_t));
  *(A->value) = ++value;

  /* Setting flat size to zero tells us the object has been written. */
  A->flat_size = 0;

  /* Create a context callback to free exobj when context is cleared */
  ctxcb = MemoryContextAlloc(objcxt, sizeof(MemoryContextCallback));

  ctxcb->func = context_callback_exobj_free;
  ctxcb->arg = A;
  MemoryContextRegisterResetCallback(objcxt, ctxcb);

  /* Switch back to old context */
  MemoryContextSwitchTo(oldcxt);
  return A;
}

/* MemoryContextCallback function to free exobj data when their
   context goes out of scope. */
static void
context_callback_exobj_free(void* ptr) {
    pgexpanded_Exobj *A = (pgexpanded_Exobj *) ptr;
    LOGF();
    pfree(A->value);
}

/* Helper function to always expanded datum

This is used by PG_GETARG_EXOBJ */
pgexpanded_Exobj *
DatumGetExobj(Datum d) {
  pgexpanded_Exobj *A;
  pgexpanded_FlatExobj *flat;
  int64_t *value;

  LOGF();
  if (VARATT_IS_EXTERNAL_EXPANDED(DatumGetPointer(d))) {
    A = ExobjGetEOHP(d);
    Assert(A->em_magic == exobj_MAGIC);
    return A;
  }
  flat = (pgexpanded_FlatExobj*)PG_DETOAST_DATUM(d);
  value = PGEXPANDED_EXOBJ_DATA(flat);
  A = new_expanded_exobj(*value, CurrentMemoryContext);
  return A;
}

Datum
exobj_in(PG_FUNCTION_ARGS) {
  char *input;
  pgexpanded_Exobj *result;
  int64_t value;
  LOGF();
  input = PG_GETARG_CSTRING(0);
  value = strtoll(input, NULL, 10);
  result = new_expanded_exobj(value, CurrentMemoryContext);
  PGEXPANDED_RETURN_EXOBJ(result);
}

Datum
exobj_out(PG_FUNCTION_ARGS)
{
  char *result;
  pgexpanded_Exobj *A = PGEXPANDED_GETARG_EXOBJ(0);
  LOGF();
  result = palloc(32);
  snprintf(result, sizeof(result), "%lld", (long long int) *A->value);
  PG_RETURN_CSTRING(result);
}

Datum
exobj_info(PG_FUNCTION_ARGS) {
  pgexpanded_Exobj *A;
  LOGF();
  A = PGEXPANDED_GETARG_EXOBJ(0);
  return Int64GetDatum(*A->value);
}

void
_PG_init(void)
{
    LOGF();
}

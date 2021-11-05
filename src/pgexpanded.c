#include "pgexpanded.h"
PG_MODULE_MAGIC;

/* Compute flattened size of storage needed to flatten an expanded
   matrix */
static Size
matrix_get_flat_size(ExpandedObjectHeader *eohptr) {
    pgexpanded_Matrix *A = (pgexpanded_Matrix*) eohptr;
    Size nbytes;
    uint64_t nvals;

    Assert(A->em_magic == matrix_MAGIC);

    /* Use cached value if already computed */
    if (A->flat_size) {
        return A->flat_size;
    }
    
    nvals = A->nvals;

    nbytes = PGEXPANDED_MATRIX_OVERHEAD();
    nbytes += nvals * sizeof(uint64_t);
    nbytes += nvals * sizeof(uint64_t);
    nbytes += nvals * sizeof(uint64_t);

    /* Cache this value in the expanded object */
    A->flat_size = nbytes;
    return nbytes;
}

/* Flatten matrix into a pre-allocated result buffer that is
   allocated_size in bytes.  */
static void
matrix_flatten_into(ExpandedObjectHeader *eohptr,
                    void *result, Size allocated_size)  {
    uint64_t nrows, ncols, nvals, *rows, *cols, *vals;

    /* Cast EOH pointer to expanded object, and result pointer to flat
       object */
    pgexpanded_Matrix *A = (pgexpanded_Matrix *) eohptr;
    pgexpanded_FlatMatrix *flat = (pgexpanded_FlatMatrix *) result;

    /* Sanity check the object is valid */
    Assert(A->em_magic == matrix_MAGIC);
    Assert(allocated_size == A->flat_size);

    /* Zero out the whole allocated buffer */
    memset(flat, 0, allocated_size);

    /* Compute the pointers to the 3 arrays in the data buffer */
    rows = PGEXPANDED_MATRIX_DATA(flat);
    cols = rows + nvals + 1;
    vals = cols + nvals;

    /* Write the struct header info */
    flat->nrows = nrows;
    flat->ncols = ncols;
    flat->nvals = nvals;

    /* If there are any values, write the 3 arrays. */
    if (nvals > 0) {
        /* Write the row data */
        memcpy(rows,
               A->rows,
               (nrows+1)*sizeof(uint64_t));

        /* Write the col data */
        memcpy(cols,
               A->cols,
               nvals*sizeof(uint64_t));

        /* Write the value data */
        memcpy(vals,
               A->vals,
               nvals*sizeof(uint64_t));
        
    }
    /* Set the final size in the header */
    SET_VARSIZE(flat, allocated_size);
}

/* Expand a flat matrix in to an Expanded one, return as Postgres Datum. */
Datum
expand_flat_matrix(pgexpanded_FlatMatrix *flat,
                   MemoryContext parentcontext) {
  pgexpanded_Matrix *A;

  MemoryContext objcxt, oldcxt;
  MemoryContextCallback *ctxcb;

  uint64_t nrows, ncols, nvals;
  uint64_t *rows, *cols, *vals;

  /* Create a new context that will hold the expanded object. */
  objcxt = AllocSetContextCreate(parentcontext,
                                 "expanded matrix",
                                 ALLOCSET_DEFAULT_SIZES);

  /* Allocate a new expanded matrix */
  A = (pgexpanded_Matrix*)MemoryContextAlloc(objcxt,
                                             sizeof(pgexpanded_Matrix));

  /* Initialize the ExpandedObjectHeader member with flattening
   * methods and the new object context */
  EOH_init_header(&A->hdr, &matrix_methods, objcxt);

  /* Used for debugging checks */
  A->em_magic = matrix_MAGIC;

  /* Switch to new object context */
  oldcxt = MemoryContextSwitchTo(objcxt);

  /* Get dimensional information from flat object */
  nrows = flat->nrows;
  ncols = flat->ncols;
  nvals = flat->nvals;

  /* Using calloc here to simulate data that would be allocated by a
     library that did the actual matrix multiplication, like
     SuiteSparse:GraphBLAS.  These arrays are then freed in the memory
     context callback function below. */
  rows = calloc(nvals, sizeof(uint64_t));
  cols = calloc(nvals, sizeof(uint64_t));
  vals = calloc(nvals, sizeof(uint64_t));
  
  memcpy(rows,
         PGEXPANDED_MATRIX_DATA(flat),
         (nrows+1)*sizeof(uint64_t));
  
  memcpy(cols,
         PGEXPANDED_MATRIX_DATA(flat) + nrows+1,
         nvals*sizeof(uint64_t));
  
  memcpy(vals,
         PGEXPANDED_MATRIX_DATA(flat) + nrows+1 + nvals,
         nvals*sizeof(uint64_t));

  /* Setting flat size to zero tells us the object has been written. */
  A->flat_size = 0;

  /* Create a context callback to free matrix when context is cleared */
  ctxcb = MemoryContextAlloc(objcxt, sizeof(MemoryContextCallback));

  ctxcb->func = context_callback_matrix_free;
  ctxcb->arg = A;
  MemoryContextRegisterResetCallback(objcxt, ctxcb);

  /* Switch back to old context */
  MemoryContextSwitchTo(oldcxt);
  PGEXPANDED_RETURN_MATRIX(A);
}

/* MemoryContextCallback function to free matrix data when their
   context goes out of scope. */
static void
context_callback_matrix_free(void* ptr) {
    pgexpanded_Matrix *A = (pgexpanded_Matrix *) ptr;
    free(A->rows);
    free(A->cols);
    free(A->vals);
}

/* Construct an empty expanded matrix. */
pgexpanded_Matrix *
construct_empty_expanded_matrix(uint64_t nrows,
                                uint64_t ncols,
                                MemoryContext parentcontext) {
    pgexpanded_FlatMatrix *flat;
    Datum d;
    flat = construct_empty_flat_matrix(nrows, ncols);
    d = expand_flat_matrix(flat, parentcontext);
    pfree(flat);
    return (pgexpanded_Matrix *) DatumGetEOHP(d);
}


/* Matrix multplication.  This doesn't actually do anything, but is
   useful to show how to do operator overloading. */
Datum
mxm(PG_FUNCTION_ARGS) {
    pgexpanded_Matrix *A, *B, *C;
    A = PGEXPANDED_GETARG_MATRIX(0);
    B = PGEXPANDED_GETARG_MATRIX(1);
    if (A->ncols != B->nrows) {
        ereport(ERROR, (errmsg("Matrix dimensions do not match")));
    }
    C = construct_empty_expanded_matrix(A->nrows, B->ncols, CurrentMemoryContext);
    PGEXPANDED_RETURN_MATRIX(C);
}

/* Datum */
/* matrix(PG_FUNCTION_ARGS) { */
/*   pgexpanded_Matrix *retval; */
/*   ArrayType *rows, *cols, *vals; */
/*   int i, count; */

/*   uint64_t *row_indices, *col_indices; */
/*   uint64_t max_row_index, max_col_index; */
/*   uint64_t *values; */
/*   ArrayIterator array_iterator; */
/*   Datum	value; */
/*   bool isnull; */

/*   if (PG_NARGS() == 2) { */
/*     max_row_index = PG_GETARG_INT64(0); */
/*     max_col_index = PG_GETARG_INT64(1); */
/*     retval = construct_empty_expanded_matrix(max_row_index, */
/*                                              max_col_index, */
/*                                              CurrentMemoryContext); */
/*     PGEXPANDED_RETURN_MATRIX(retval); */
/*   } */

/*   rows = PG_GETARG_ARRAYTYPE_P(0); */
/*   cols = PG_GETARG_ARRAYTYPE_P(1); */
/*   vals = PG_GETARG_ARRAYTYPE_P(2); */

/*   if (ARR_HASNULL(rows) || ARR_HASNULL(cols) || ARR_HASNULL(vals)) */
/*     ereport(ERROR, (errmsg("Matrix values may not be null"))); */

/*   idims = ARR_DIMS(rows); */
/*   jdims = ARR_DIMS(cols); */
/*   vdims = ARR_DIMS(vals); */

/*   if (!PG_ARGISNULL(3)) */
/*     max_row_index = PG_GETARG_INT64(3); */
/*   else */
/*     max_row_index = idims[0]; */

/*   if (!PG_ARGISNULL(4)) */
/*     max_col_index = PG_GETARG_INT64(4); */
/*   else */
/*     max_col_index = jdims[0]; */

/*   count = vdims[0]; */

/*   if ((idims[0] != jdims[0]) || (jdims[0] != vdims[0])) */
/*     ereport(ERROR, (errmsg("Row, column, and value arrays must be same size."))); */

/*   row_indices = (uint64_t*) palloc0(sizeof(uint64_t) * max_row_index); */
/*   col_indices = (uint64_t*) palloc0(sizeof(uint64_t) * max_col_index); */
/*   values = (uint64_t*) palloc0(sizeof(uint64_t) * count); */

/*   i = 0; */
/*   array_iterator = array_create_iterator(rows, 0, NULL); */
/*   while (array_iterate(array_iterator, &value, &isnull)) */
/* 	{ */
/*       row_indices[i] = DatumGetInt64(value); */
/*       if (row_indices[i] > max_row_index) */
/*         max_row_index = row_indices[i] + 1; */
/*       i++; */
/*   } */
/*   array_free_iterator(array_iterator); */

/*   i = 0; */
/*   array_iterator = array_create_iterator(cols, 0, NULL); */
/*   while (array_iterate(array_iterator, &value, &isnull)) */
/* 	{ */
/*       col_indices[i] = DatumGetInt64(value); */
/*       if (col_indices[i] > max_col_index) */
/*         max_col_index = col_indices[i] + 1; */
/*       i++; */
/*   } */
/*   array_free_iterator(array_iterator); */

/*   i = 0; */
/*   array_iterator = array_create_iterator(vals, 0, NULL); */
/*   while (array_iterate(array_iterator, &value, &isnull)) */
/* 	{ */
/*       values[i] = DatumGetInt64(value); */
/*       i++; */
/*   } */
/*   array_free_iterator(array_iterator); */

/*   retval = construct_empty_expanded_matrix(max_row_index, */
/*                                                max_col_index, */
/*                                                CurrentMemoryContext); */

/*   CHECKD(GrB_Matrix_build(retval->M, */
/*                           row_indices, */
/*                           col_indices, */
/*                           values, */
/*                           count, */
/*                           GB_DUP)); */

/*   PGEXPANDED_RETURN_MATRIX(retval); */
/* } */

/* Construct an empty flat matrix. */
pgexpanded_FlatMatrix *
construct_empty_flat_matrix(uint64_t nrows, uint64_t ncols) {
  pgexpanded_FlatMatrix *result;
  result = (pgexpanded_FlatMatrix *) palloc0(sizeof(pgexpanded_FlatMatrix));
  SET_VARSIZE(result, sizeof(pgexpanded_FlatMatrix));
  result->nvals = 0;
  return result;
}

/* Helper function to always expanded datum

This is used by PG_GETARG_MATRIX */
pgexpanded_Matrix *
DatumGetMatrix(Datum d) {
  pgexpanded_Matrix *A;
  pgexpanded_FlatMatrix *flat;

  if (VARATT_IS_EXTERNAL_EXPANDED(DatumGetPointer(d))) {
    A = MatrixGetEOHP(d);
    Assert(A->em_magic == matrix_MAGIC);
    return A;
  }
  flat = (pgexpanded_FlatMatrix*)PG_DETOAST_DATUM(d);
  expand_flat_matrix(flat, CurrentMemoryContext);
  return MatrixGetEOHP(d);
}

Datum
matrix_in(PG_FUNCTION_ARGS) {
  pgexpanded_FlatMatrix *flat;
  char *input;
  size_t len;
  int bc;
  Datum d;

  input = PG_GETARG_CSTRING(0);
  len = strlen(input);
  bc = (len) / 2 + VARHDRSZ;
  flat = palloc(bc);
  hex_decode(input, len, (char*)flat);
  d = expand_flat_matrix(flat, CurrentMemoryContext);
  return d;
}

Datum
matrix_out(PG_FUNCTION_ARGS)
{
  Size size;
  char *rp, *result, *buf;
  pgexpanded_Matrix *A = PGEXPANDED_GETARG_MATRIX(0);
  size = EOH_get_flat_size(&A->hdr);
  buf = palloc(size);
  EOH_flatten_into(&A->hdr, buf, size);
  rp = result = palloc((size * 2) + 1);
  rp += hex_encode(buf, size, rp);
  *rp = '\0';
  PG_RETURN_CSTRING(result);
}

Datum
matrix_nrows(PG_FUNCTION_ARGS) {
  pgexpanded_Matrix *mat;
  uint64_t count;
  mat = PGEXPANDED_GETARG_MATRIX(0);
  count = 0;
  return Int64GetDatum(count);
}

Datum
matrix_ncols(PG_FUNCTION_ARGS) {
  pgexpanded_Matrix *mat;
  uint64_t count;
  mat = PGEXPANDED_GETARG_MATRIX(0);
  count = 0;
  return Int64GetDatum(count);
}

Datum
matrix_nvals(PG_FUNCTION_ARGS) {
  pgexpanded_Matrix *mat;
  uint64_t count;
  mat = PGEXPANDED_GETARG_MATRIX(0);
  count = 0;
  return Int64GetDatum(count);
}


void
_PG_init(void)
{
}

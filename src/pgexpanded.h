#ifndef PGEXPANDED_H
#define PGEXPANDED_H

#include "postgres.h"
#include "utils/builtins.h"
#include "libpq/pqformat.h"
#include "funcapi.h"
#include "access/htup_details.h"
#include "utils/array.h"
#include "utils/arrayaccess.h"
#include "catalog/pg_type_d.h"
#include "catalog/pg_type.h"
#include "utils/lsyscache.h"
#include "nodes/pg_list.h"
#include "utils/varlena.h"

/* dumb debug helper */
#define elogn(s) elog(NOTICE, "%s", (s))
#define elogn1(s, v) elog(NOTICE, "%s: %lu", (s), (v))

/* ID for debugging crosschecks */
#define matrix_MAGIC 689276813

/* Flattened representation of matrix, used to store to disk.

The first 32 bits must the length of the data.  Actual array data is
appended after this struct and cannot exceed 1GB.
*/
typedef struct pgexpanded_FlatMatrix {
    int32 vl_len_;
    
    uint64_t nrows;
    uint64_t ncols;
    uint64_t nvals;
} pgexpanded_FlatMatrix;

/* Expanded representation of matrix.

When loaded from storage, the flattened representation is used to
build the matrix.

This is a simple coordinate storage format of 3 equal sized unsigned
integer arrays.
*/
typedef struct pgexpanded_Matrix  {
    ExpandedObjectHeader hdr;
    int em_magic;
    
    uint64_t nrows;
    uint64_t ncols;
    uint64_t nvals;
    
    uint64_t *rows;
    uint64_t *cols;
    uint64_t *vals;
    
    Size     flat_size;
} pgexpanded_Matrix;

/* Callback function for freeing matrix arrays. */
static void
context_callback_matrix_free(void*);

/* Expanded Object Header "methods" for flattening matrices for storage */
static Size
matrix_get_flat_size(ExpandedObjectHeader *eohptr);

static void
matrix_flatten_into(ExpandedObjectHeader *eohptr,
                    void *result, Size allocated_size);

static const ExpandedObjectMethods matrix_methods = {
     matrix_get_flat_size,
     matrix_flatten_into
    };

/* Utility function that expands a flattened matrix datum. */
Datum
expand_flat_matrix(pgexpanded_FlatMatrix *flat,
                   MemoryContext parentcontext);

pgexpanded_Matrix *
construct_empty_expanded_matrix(uint64_t nrows,
                                uint64_t ncols,
                                MemoryContext parentcontext);

/* Helper function that either detoasts or expands matrices. */
pgexpanded_Matrix *
DatumGetMatrix(Datum d);

/* Helper function to create an empty flattened matrix. */
pgexpanded_FlatMatrix *
construct_empty_flat_matrix(uint64_t nrows, uint64_t ncols);

/* Helper macro to detoast and expand matrixs arguments */
#define PGEXPANDED_GETARG_MATRIX(n)  DatumGetMatrix(PG_GETARG_DATUM(n))

/* Helper macro to return Expanded Object Header Pointer from matrix. */
#define PGEXPANDED_RETURN_MATRIX(A) return EOHPGetRWDatum(&(A)->hdr)

/* Helper macro to compute flat matrix header size */
#define PGEXPANDED_MATRIX_OVERHEAD() MAXALIGN(sizeof(pgexpanded_FlatMatrix))

/* Helper macro to get pointer to beginning of matrix data. */
#define PGEXPANDED_MATRIX_DATA(a) ((uint64_t *)(((char *) (a)) + PGEXPANDED_MATRIX_OVERHEAD()))

/* Help macro to cast generic Datum header pointer to expanded Matrix */
#define MatrixGetEOHP(d) (pgexpanded_Matrix *) DatumGetEOHP(d);

/* Public API functions */

PG_FUNCTION_INFO_V1(matrix);
PG_FUNCTION_INFO_V1(matrix_in);
PG_FUNCTION_INFO_V1(matrix_out);

PG_FUNCTION_INFO_V1(matrix_ncols);
PG_FUNCTION_INFO_V1(matrix_nrows);
PG_FUNCTION_INFO_V1(matrix_nvals);

PG_FUNCTION_INFO_V1(mxm);

void
_PG_init(void);

#endif /* PGEXPANDED_H */

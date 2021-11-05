-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pggraphblas" to load this file. \quit

CREATE TYPE matrix;

CREATE FUNCTION matrix_in(cstring)
RETURNS matrix
AS '$libdir/pggraphblas', 'matrix_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION matrix_out(matrix)
RETURNS cstring
AS '$libdir/pggraphblas', 'matrix_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION nrows(matrix)
RETURNS bigint
AS '$libdir/pggraphblas', 'matrix_nrows'
LANGUAGE C STABLE;

CREATE FUNCTION ncols(matrix)
RETURNS bigint
AS '$libdir/pggraphblas', 'matrix_ncols'
LANGUAGE C STABLE;

CREATE FUNCTION nvals(matrix)
RETURNS bigint
AS '$libdir/pggraphblas', 'matrix_nvals'
LANGUAGE C STABLE;

-- CREATE FUNCTION matrix(bigint[], bigint[], bigint[],
--     bigint default null, bigint default null)
-- RETURNS matrix
-- AS '$libdir/pggraphblas', 'matrix_int64'
-- LANGUAGE C STABLE;

CREATE FUNCTION mxm(
    A matrix,
    B matrix,
    )
RETURNS matrix
AS '$libdir/pggraphblas', 'mxm'
LANGUAGE C STABLE;

-- matrix operators

CREATE OPERATOR * (
    leftarg = matrix,
    rightarg = matrix,
    procedure = mxm
);

-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pgexpanded" to load this file. \quit

CREATE TYPE matrix;

CREATE FUNCTION matrix_in(cstring)
RETURNS matrix
AS '$libdir/pgexpanded', 'matrix_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION matrix_out(matrix)
RETURNS cstring
AS '$libdir/pgexpanded', 'matrix_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE matrix (
    input = matrix_in,
    output = matrix_out,
    alignment = int4,
    storage = 'extended',
    internallength = -1
);

CREATE FUNCTION nrows(matrix)
RETURNS bigint
AS '$libdir/pgexpanded', 'matrix_nrows'
LANGUAGE C STABLE;

CREATE FUNCTION ncols(matrix)
RETURNS bigint
AS '$libdir/pgexpanded', 'matrix_ncols'
LANGUAGE C STABLE;

CREATE FUNCTION nvals(matrix)
RETURNS bigint
AS '$libdir/pgexpanded', 'matrix_nvals'
LANGUAGE C STABLE;

-- CREATE FUNCTION matrix(bigint[], bigint[], bigint[],
--     bigint default null, bigint default null)
-- RETURNS matrix
-- AS '$libdir/pgexpanded', 'matrix_int64'
-- LANGUAGE C STABLE;

CREATE FUNCTION mxm(
    A matrix,
    B matrix
    )
RETURNS matrix
AS '$libdir/pgexpanded', 'mxm'
LANGUAGE C STABLE;

-- matrix operators

CREATE OPERATOR * (
    leftarg = matrix,
    rightarg = matrix,
    procedure = mxm
);

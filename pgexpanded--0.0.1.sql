-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION pgexpanded" to load this file. \quit

CREATE TYPE exobj;

CREATE FUNCTION exobj_in(cstring)
RETURNS exobj
AS '$libdir/pgexpanded', 'exobj_in'
LANGUAGE C IMMUTABLE STRICT;

CREATE FUNCTION exobj_out(exobj)
RETURNS cstring
AS '$libdir/pgexpanded', 'exobj_out'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE exobj (
    input = exobj_in,
    output = exobj_out,
    alignment = int4,
    storage = 'extended',
    internallength = -1
);

CREATE FUNCTION info(exobj)
RETURNS bigint
AS '$libdir/pgexpanded', 'exobj_info'
LANGUAGE C STABLE;

create or replace function test_expand(obj exobj) returns exobj language plpgsql as
    $$
    declare
        i bigint = info(obj);
    begin
        raise notice 'expand count %', i;
        return obj;
    end;
    $$;

create or replace function test_expand_expand(obj exobj) returns exobj language plpgsql as
    $$
    declare
        i bigint = info(obj);
    begin
        raise notice 'expand expand count %', i;
        obj = test_expand(obj);
        return test_expand(obj);
    end;
    $$;

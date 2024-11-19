# pgexpanded

This is an example postgres extension that shows how to implement an
"expanded" data type in C as described [in this
documentation](https://www.postgresql.org/docs/current/xtypes.html):

*"Another feature that's enabled by TOAST support is the possibility of
having an expanded in-memory data representation that is more
convenient to work with than the format that is stored on disk. The
regular or “flat” varlena storage format is ultimately just a blob of
bytes; it cannot for example contain pointers, since it may get copied
to other locations in memory. For complex data types, the flat format
may be quite expensive to work with, so PostgreSQL provides a way to
“expand” the flat format into a representation that is more suited to
computation, and then pass that format in-memory between functions of
the data type."*

This repository provides a simple, compilable and runnable example
expanded data type that can be used as a basis for other extensions.
By way of trivial example, it shows how to expand a data type that
keeps track of the number of expansions it's gone through.

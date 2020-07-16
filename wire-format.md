Venus Wire Format
=================

A command stream consists of a sequence of command requests.  When requested,
the renderer can generate a reply stream in response to a command stream.
Each reply stream consists of a sequence of command replies.

## Scalars

A fixed-width integer value is encoded in little endian byte order, and is
padded to 32-bit.

A floating-point value is reinterpreted to an unsigned fixed-width integer
value of the same widths and is encoded as the unsigned fixed-width integer
value.

An enum is casted to `int32_t` and is encoded as `int32_t`.

`size_t` is casted to `uint64_t` and is encoded as `uint64_t`.

## Object Handles

An object handle, dispatchable or not, is encoded as a `uint64_t`, indicating
the object id.  `VK_NULL_HANDLE` has object id 0.

## External Handles

An external handles of types

 - `VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT`
 - `VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT`
 - `VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT`

can be encoded as a `uint64_t`, indicating the resource id.

## Pointers

A pointer is always considered a dynamically-sized array.  A `uint64_t` is
encoded first to indicate the number of array elements encoded, followed by
the array elements.  Padding to 32-bit, if needed, happens after the entire
array is encoded, not after each individual array element.

A `NULL` pointer is encoded as an array of size 0.  Conversely, an array of
size 0 must be decoded to a `NULL` pointer.  (Is this going to be an issue?)

A blob pointer is encoded as a `uint8_t` pointer.  A string pointer is encoded
as a blob pointer, up to the first null byte of the string.

`pNext` is not considered a pointer.

## Fixed-Size Arrays

A fixed-size array is encoded the same way as a pointer.  The number of array
elements encoded must be less than or equal to the fixed size.

## Structs

For a plain struct, all data members are encoded in order.

For a `pNext` chain, all data members except `pNext` are encoded in order
first.  If `pNext` is non-`NULL`, the next node in the chain is then encoded.
If `pNext` is `NULL`, `VK_STRUCTURE_TYPE_MAX_ENUM` is encoded to terminate the
chain.

## Unions

For a union, a `uint32_t` is encoded first to indicate the index of the data
member encoded, followed by the data member.

## Others

All other types of values, including but not limited to

 - function pointers
 - pointers to mapped memory object regions
 - external handles of types not listed above
 - WSI handles

are not serializable.

## Command Requests

For a command request, a `VnCommandType` is encoded first followed by
`VnCommandFlags` (merge them together?).  Command parameters are then encoded
in order.

When a command parameter is a non-const pointer/array, the value(s) pointed to
is assumed to be uninitialized except for

 - `sType`
 - `pNext`
 - object handles
 - `uint32_t` indicating the size of another command parameter

A special rule must be followed to encode only the initialized parts.  (More
specific?  The generated code likely have bugs as well.)

## Command Replies

For a command reply, a `VnCommandType` is encoded first.  If the command has a
return value, the return value is then encoded.  Finally, command parameters
that are non-const pointers/arrays are encoded in order.

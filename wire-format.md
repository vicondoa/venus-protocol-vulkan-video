Venus Wire Format
=================

The renderer accepts streams of command requests, where each stream consists
of a sequence of command requests.

The renderer can generate streams of command replies in response, when
requested.  Each reply stream consists of a sequence of command replies.

## Scalars

Fixed-width integer values are encoded in little endian byte order, and then
padded to 32-bit.

Floating-point values are reinterpreted to unsigned fixed-width integer
values of the same widths and are encoded as unsigned fixed-width integer
values.

`size_t` is casted to `uint64_t` and is encoded as `uint64_t`.

`char` is casted to `uint8_t` and is encoded as `uint8_t`.

## Object Handles

An object handle, dispatchable or not, is encoded as a `uint64_t`, indicating
the object id.  `VK_NULL_HANDLE` has object id 0.

## External Handles

An external handles of types

 - `VK_EXTERNAL_MEMORY_HANDLE_TYPE_OPAQUE_FD_BIT`
 - `VK_EXTERNAL_FENCE_HANDLE_TYPE_OPAQUE_FD_BIT`
 - `VK_EXTERNAL_SEMAPHORE_HANDLE_TYPE_OPAQUE_FD_BIT`

can be encoded as a `uint64_t`, indicating the resource id.

## Structs

For a plain struct, all data members are encoded in order.

For a `pNext` chain, all data members except `pNext` are encoded in order
first.  If `pNext` is non-`NULL`, the next node in the chain is then encoded.
If `pNext` is `NULL`, `VK_STRUCTURE_TYPE_MAX_ENUM` is encoded to terminate the
chain.

## Unions

For a union, a `uint32_t` is encoded first to indicate the index of the data
member encoded, followed by the data member.

## Fixed-Size Arrays

For a fixed-size array, a `uint64_t` is encoded first to indicate the number
of array elements, followed by the array elements.  The number of array
elements encoded must be less than or equal to the size of the fixed-size
array.

Padding to 32-bit, if needed, happens after the entire array is encoded, not
after each individual array element.

## Pointers

For a pointer, a `uint64_t` is encoded first to indicate the address of the
pointer.  If non-NULL, the value pointed to is then encoded.  (Pointers to
arrays are common.  Do we want to encode both the pointer address and the
array size?  Can we say that pointers are just arrays?  We wouldn't be able to
distinguish NULL and a zero-element array though.)

A string pointer is encoded up to the first null byte of the string.

A blob pointer is encoded as a `uint8_t` pointer.

`pNext` is not considered a pointer.

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

When a command parameter is a non-const pointer/array, the value pointed to is
assumed to be uninitialized except for

 - `sType`
 - `pNext`
 - object handles
 - `uint32_t` indicating the size of another command parameter

A special rule must be followed to encode only initialized parts.  (Do we want
this?  The generated code seems to have bugs.)

## Command Replies

For a command reply, a `VnCommandType` is encoded first followed by a
`VkResult` (why?).  If the command has a return value other than `VkResult`,
the return value is then encoded.  Finally, command parameters that are
non-const pointers/arrays are encoded in order.

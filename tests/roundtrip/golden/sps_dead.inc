/* Byte offsets within StdVideoH264SequenceParameterSet that do not influence
 * the encoded bytes.
 *
 * StdVideoH264SpsFlags occupies offsets 0..3 and declares 13 single-bit
 * fields, so only bits 0..12 -- offsets 0 and 1 -- carry information. Offsets
 * 2 and 3 are the reserved remainder of the bitfield word.
 *
 * That these two offsets are dead is the property we want: the wire carries
 * exactly the flags the schema names, and nothing that depends on how a
 * particular compiler chose to lay out the reserved bits. If a real field
 * ever stops reaching the wire it shows up here as a new offset and the test
 * fails.
 */
2, 3

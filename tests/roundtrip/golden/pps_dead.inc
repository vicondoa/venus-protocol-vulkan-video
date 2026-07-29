/* Byte offsets within StdVideoH264PictureParameterSet that do not influence
 * the encoded bytes.
 *
 * StdVideoH264PpsFlags occupies offsets 0..3 and declares 8 single-bit
 * fields, so only bits 0..7 -- offset 0 -- carry information. Offsets 1, 2
 * and 3 are the reserved remainder of the bitfield word.
 *
 * See sps_dead.inc for why a growing list here is a failure, not a nuisance.
 */
1, 2, 3

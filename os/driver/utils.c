#include "type.h"
#include "utils.h"

void set_bits_value(volatile uint32 *bits, uint32 mask, uint32 value)
{
    uint32 masked_origin_value = (*bits) & ~mask;
    *bits = masked_origin_value | (value & mask);
}

void set_bits_value_offset(volatile uint32 *bits, uint32 mask, uint32 value, uint32 offset)
{
    set_bits_value(bits, mask << offset, value << offset);
}

void set_bit(volatile uint32 *bits, uint32 offset)
{
    set_bits_value(bits, 1 << offset, 1 << offset);
}

void clear_bit(volatile uint32 *bits, uint32 offset)
{
    set_bits_value(bits, 1 << offset, 0);
}
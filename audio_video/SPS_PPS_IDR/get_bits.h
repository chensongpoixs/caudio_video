#ifndef  _GET_BITS_H
#define  _GET_BITS_H


#include <stdint.h>

/**
 * Read 1-25 bits.
 */
static inline unsigned int get_bits(uint8_t *buffer, unsigned *index,  int n)
{
	uint8_t result = 0;
	// 找到8个bit位置
	for (int i = 0; i < n; ++i)
	{
		uint8_t k = buffer[(*index) >> 3];

		/*k <<= (*index) & 0x07;
		k >>= 8 - 1;*/


		result >>= (*index) & 7;
		result &= 1;

		++(*index);

		result = result << 1 & k;
	}
    return result;
}


#endif // _GET_BITS_H
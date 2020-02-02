#include <stdint.h>

typedef uint16_t u16;
typedef uint32_t u32;

inline u32 swap32(u32 swapme)
{
	return (((swapme & 0xFF000000) >> 24)
		   |((swapme & 0x00FF0000) >>  8)
           |((swapme & 0x0000FF00) <<  8)
		   |((swapme & 0x000000FF) << 24));
}
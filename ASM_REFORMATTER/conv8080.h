#ifndef CONV8080_H
#define CONV8080_H

#include "reformatter.h"

//=============================================================================
//		convert Z-80 to 8080
//=============================================================================

// Converts Z-80 instructions to their 8080 equivalents, modifying the tokens in place.
void conv8080( tokens_t *pTokens, options_t *popts );

#endif // CONV8080_H

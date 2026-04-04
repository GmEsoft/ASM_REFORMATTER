#ifndef CONVZ80_H
#define CONVZ80_H

#include "reformatter.h"

//=============================================================================
//		convert 8080 to Z-80
//=============================================================================


// Converts 8080 instructions to their Z-80 equivalents, modifying the tokens in place.
void convz80( tokens_t *pTokens, options_t *popts );

#endif // CONVZ80_H

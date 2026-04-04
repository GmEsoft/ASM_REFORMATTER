#ifndef REFORMATTER_H
#define REFORMATTER_H

#include <stdio.h>

#define TRACE 0
#define TRC if(TRACE)

typedef unsigned int uint32_t;
#define false 0
#define true 1

//=============================================================================
//		options_t
//=============================================================================
typedef struct
{
	int		tabsize;				// Tab size (dft: 8)
	int		comment_tab;			// Comments tab (dft: 4)
	int		mnemo_tab;				// Mnemonics tab (dft: 2)
	int		ucase;					// Convert to upper-case (except literals and comments)
	int		lcase;					// Convert to lower-case (except literals and comments)
	int		sep;					// Multi-statements separator ('!' with some assemblers)
	int		sep_in_comment;			// Allow 'sep' in comments
	int		noinstr_comments;		// Align no-instr comments with mnemonics
	int		conv_z80;				// Convert 8080 code to Z-80
	int		conv_8080;				// Convert Z-80 code to 8080 (not implemented yet)
	int		conv_z80lib;			// Convert 8080 code to Z-80 or vice-versa using z80lib conventions (not implemented yet)
	int		mark_unknown;			// Mark unrecognized instructions with a special character (e.g., '*' or '#')
	int		eof;					// Add EOF code at end of file
} options_t;


//=============================================================================
//		str_t: a simple string class with fixed max length
//=============================================================================
typedef struct
{
	char	s[256];		// string buffer (max 255 chars + null terminator)
} str_t;

// Initializes the string to an empty state.
void str_init( str_t *pStr );

// Returns a pointer to the C-string contained in the str_t structure.
char *str_cstr( str_t *pStr );

// Appends a character to the string if it does not exceed the maximum length.
void str_putc( str_t *pStr, int c );

// Trims trailing spaces and tabs from the string.
void str_trim( str_t *pStr );

//=============================================================================
//		tokens_t: a structure to hold up to 4 tokens
//		(label, mnemonic, args, comment) for an assembly line
//=============================================================================
typedef struct
{
	str_t	tokens[4];	// 0: label, 1: mnemonic, 2: args, 3: comment
	int		ntokens;	// number of tokens currently in use (0 to 3)
} tokens_t;

// Initializes the tokens structure to an empty state.
void tokens_init( tokens_t *pTokens );

// Returns the number of tokens currently in use.
int tokens_count( tokens_t *pTokens );

// Returns a pointer to the str_t structure for the specified token.
str_t *tokens_get( tokens_t *pTokens, int n );

// Returns a pointer to the C-string of the specified token.
char *tokens_getcstr( tokens_t *pTokens, int n );

// Returns a pointer to the C-string of the specified token.
char *tokens_getcstr( tokens_t *pTokens, int n );

// Returns true if there are no tokens and the first token is empty.
int tokens_empty( tokens_t *pTokens );

// Appends a character to the current token (the last one in use).
void tokens_putc( tokens_t *pTokens, int c );

// Moves to the next token, initializing it if there are less than 3 tokens in use.
void tokens_next( tokens_t *pTokens );

// Sets the number of tokens in use, initializing the new token if needed.
void tokens_set( tokens_t *pTokens, int n );

// Splits the args string into tokens based on commas, respecting quotes and comments.
void tokens_splitargs( tokens_t *pTokens, char *args );

// Writes the tokens to the output file with proper alignment based on the options.
void tokens_write( tokens_t *pTokens, FILE *outfile, options_t *popts );

// Sets the number of tokens in use, initializing the new token if needed.
void tokens_set( tokens_t *pTokens, int n );

// Splits the args string into tokens based on commas, respecting quotes and comments.
void tokens_splitargs( tokens_t *pTokens, char *args );

// Writes the tokens to the output file with proper alignment based on the options.
void tokens_write( tokens_t *pTokens, FILE *outfile, options_t *popts );

//=============================================================================
//		reformat: reads the input file character by character,
//		tokenizes it into labels, mnemonics, arguments, and comments,
//		applies formatting and optional 8080 to Z-80 conversion,
//		and writes the output file.
//=============================================================================
void reformat( FILE *infile, FILE *outfile, options_t *popts );

#endif // REFORMATTER_H

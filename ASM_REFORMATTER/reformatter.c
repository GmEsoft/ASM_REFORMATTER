#define _CRT_SECURE_NO_WARNINGS // to avoid warnings about unsafe functions like strcpy, strcat, etc.

#include "reformatter.h"
#include "convz80.h"
#include "conv8080.h"

#include <string.h> // strlen, strcpy
#include <ctype.h>  // toupper, tolower


// Note: the functions below do not check for null pointers,
// as they are only used internally and always with valid pointers.
// If needed, checks can be added for robustness,
// but they are omitted here for simplicity and performance.

// Initializes the string to an empty state.
void str_init( str_t *pStr )
{
	pStr->s[0] = 0;
}

// Returns a pointer to the C-string contained in the str_t structure.
char *str_cstr( str_t *pStr )
{
	return pStr->s;
}

// Appends a character to the string if it does not exceed the maximum length.
void str_putc( str_t *pStr, int c )
{
	int p = strlen( pStr->s );
	if ( p < 255 )
	{
		pStr->s[p++] = (char)c;
		pStr->s[p++] = 0;
	}
	else
	{
		puts( "String too long:" );
		puts( pStr->s );
	}
}

// Trims trailing spaces and tabs from the string.
void str_trim( str_t *pStr )
{
	int p = strlen( pStr->s ) - 1;
	while ( p > 0 && ( pStr->s[p] == ' ' || pStr->s[p] == '\t' ) )
	{
		pStr->s[p--] = 0;
	}
}

// Initializes the tokens structure to an empty state.
void tokens_init( tokens_t *pTokens )
{
	pTokens->ntokens = 0;
	for ( int i=0; i<4; ++i )
	{
		str_init( &pTokens->tokens[i] );
	}
}

// Returns the number of tokens currently in use.
int tokens_count( tokens_t *pTokens )
{
	return pTokens->ntokens;
}

// Returns a pointer to the str_t structure for the specified token.
str_t *tokens_get( tokens_t *pTokens, int n )
{
	if ( n > pTokens->ntokens )
	{
		puts( "tokens_get: bad token number" );
		return 0;
	}
	return &pTokens->tokens[n];
}

// Returns a pointer to the C-string of the specified token.
char *tokens_getcstr( tokens_t *pTokens, int n )
{
	if ( n > pTokens->ntokens )
	{
		puts( "tokens_getcstr: bad token number" );
		return 0;
	}
	return str_cstr( &pTokens->tokens[n] );
}

// Returns true if there are no tokens and the first token is empty.
int tokens_empty( tokens_t *pTokens )
{
	return !( pTokens->ntokens ) && !( tokens_getcstr( pTokens, 0 )[0] );
}

// Appends a character to the current token (the last one in use).
void tokens_putc( tokens_t *pTokens, int c )
{
	str_putc( &pTokens->tokens[pTokens->ntokens], c );
}

// Moves to the next token, initializing it if there are less than 3 tokens in use.
void tokens_next( tokens_t *pTokens )
{
	if ( pTokens->ntokens < 3 )
	{
		++pTokens->ntokens;
		str_init( &pTokens->tokens[pTokens->ntokens] );
	}
	else
	{
		puts( "tokens_next: too many tokens" );
	}
}

// Sets the number of tokens in use, initializing the new token if needed.
void tokens_set( tokens_t *pTokens, int n )
{
	if ( n <= 3 )
	{
		pTokens->ntokens = n;
		str_init( &pTokens->tokens[pTokens->ntokens] );
	}
	else
	{
		puts( "tokens_set: bad token number" );
	}
}

// Splits the args string into tokens based on commas, respecting quotes and comments.
void tokens_splitargs( tokens_t *pTokens, char *args )
{
	char *p0 = args;
	char *p = p0;
	char quot = 0;

	tokens_init( pTokens );

	while ( *p )
	{
		while ( *p && *p != ',' )
		{
			int esc = false;
			do
			{
				if ( esc )
				{
					esc = false;
				}
				else if ( *p == '\\' )
				{
					esc = true;
				}
				else if ( quot )
				{
					if ( !*p || quot == *p )
						quot = 0;
				}
				else if ( *p == '\'' || *p == '"' )
				{
					quot = *p;
				}

				if ( quot )
				{
					tokens_putc( pTokens, *p );
					++p;
				}
			}
			while ( *p && quot );

			if ( p > p0 || ( *p != ' ' && *p != '\t' ) )
				tokens_putc( pTokens, *p );
			++p;
		}

		str_trim( tokens_get( pTokens, pTokens->ntokens ) );

		if ( *p && *p == ',' )
		{
			++p;
		}

		p0 = p;

		if ( tokens_count( pTokens ) < 3 )
			tokens_next( pTokens );
		else
			tokens_putc( pTokens, ',' );
	}
}

// Writes the tokens to the output file with proper alignment based on the options.
void tokens_write( tokens_t *pTokens, FILE *outfile, options_t *popts )
{
	int tab = 0;
	int pos = 0;
	int len = 0;
	int lastc = 0;
	int tabsize = popts->tabsize;
	int mnemo_tab = popts->mnemo_tab;
	int comment_tab = popts->comment_tab;
	char *s;

	while ( pTokens->ntokens && !*tokens_getcstr( pTokens, pTokens->ntokens ) )
	{
		--pTokens->ntokens;
	}

	for ( int i=0; i<=pTokens->ntokens; ++i )
	{
		str_trim( &pTokens->tokens[i] );
		s = tokens_getcstr( pTokens, i );

		switch ( i )
		{
		case 0:
			tab = 0;
			break;
		case 1:
			tab = mnemo_tab;
			break;
		case 3:
			tab = comment_tab;
			break;
		default:
			++tab;
		}

		if ( i > 0 && pos >= tab * tabsize && lastc != ':' && s[0] != ';' )
		{
			// Add a space if not the leftmost column, on or past the tab,
			// not after a label and not before a comment
			putc( ' ', outfile );
			++pos;
		}
		else while ( pos < tab * tabsize )
		{
			putc( '\t', outfile );
			pos += tabsize - ( pos % tabsize );
		}

		fputs( s, outfile );
		len = strlen( s );
		pos += len;
		lastc = len ? s[len-1] : 0;
	}
	fputc( '\n', outfile );
}

//=============================================================================
//		reformat: reads the input file character by character,
//		tokenizes it into labels, mnemonics, arguments, and comments,
//		applies formatting and optional 8080 to Z-80 conversion,
//		and writes the output file.
//=============================================================================
void reformat( FILE *infile, FILE *outfile, options_t *popts )
{
	int c, lastc = 0;
	int quote = 0;
	int cmt = 0;
	tokens_t tokens;

	tokens_init( &tokens );

	// Read the input file character by character and process it
	// according to the current state (in comment, in quote, etc.).
	while ( ( c = fgetc( infile ) ) != EOF )
	{
		// If we are currently in a comment or a quoted string, we handle characters differently.
		if ( cmt || quote )
		{
			// If 'sep_in_comment' option is enabled and we encounter
			// the separator character, we treat it as a newline.
			if ( c && popts->sep_in_comment && c == popts->sep )
			{
				c = 0x0A;
			}

			// If we encounter a tab character, we add it to the current token.
			if ( c == '\t' )
			{
				tokens_putc( &tokens, c );
			}
			// If we encounter a printable character, we add it to the current token,
			// and if it's a quote character, we toggle the quote state.
			else if ( c >= ' ' )
			{
				if ( c == quote )
					quote = 0;
				tokens_putc( &tokens, c );
			}
			// If we encounter a newline character, we end the current line,
			// write the tokens to the output file, and reset the tokens for the next line.
			else if ( c == 0x0D || c == 0x0A )
			{
				quote = cmt = 0;
				if ( popts->conv_z80 )
					convz80( &tokens, popts );
				else if ( popts->conv_8080 )
					conv8080( &tokens, popts );
				tokens_write( &tokens, outfile, popts );
				tokens_init( &tokens );
			}
		}
		// If we are not in a comment or a quoted string, we handle characters according
		// to their role in assembly syntax.
		else
		{
			// If 'sep' option is set and we encounter the separator character,
			// we treat it as a newline.
			if ( c && c == popts->sep )
			{
				c = 0x0A;
			}

			// Ignore single quote following an instruction as it is likely a Z-80 AF' register, which the parser splits into AF and '.
			if ( c == '\'' && isalpha( lastc ) )
			{
				c |= 0x100; // Mark the single quote as a special character to ignore it in the main parsing loop.
			}

			// We handle different characters based on their role in assembly syntax.
			switch ( c )
			{
			case 0x0D:
			case 0x0A: // Newline characters
				quote = cmt = 0;
				if ( popts->conv_z80 )
					convz80( &tokens, popts );
				else if ( popts->conv_8080 )
					conv8080( &tokens, popts );
				tokens_write( &tokens, outfile, popts );
				tokens_init( &tokens );
				break;
			case ';': // Comment character
				cmt = 1;
				tokens_set( &tokens,
					tokens_empty( &tokens )																		? 0
					: popts->noinstr_comments && tokens_count( &tokens ) < 2 && !tokens_getcstr( &tokens, 1 )[0] ? 1
																												: 3 );
				tokens_putc( &tokens, c );
				break;
			case '"':
			case '\'': // Quote characters
				quote = c;
				tokens_putc( &tokens, c );
				break;
			case ':': // Label separator
				if ( tokens_count( &tokens ) == 1 && !tokens_getcstr( &tokens, 0 )[0] )
				{
					strcpy( tokens_getcstr( &tokens, 0 ), tokens_getcstr( &tokens, 1 ) );
					tokens.ntokens = 0;
					// *tokens_get( &tokens, 0 ) = *tokens_get( &tokens, 1 );
				}
				tokens_putc( &tokens, c );
				// fall into
			case ' ':
			case '\t': // Space and tab characters
				// If we have less than 2 tokens, we skip consecutive spaces and tabs,
				// and if we reach the end of the line, we break out of the loop.
				if ( tokens_count( &tokens ) < 2 )
				{
					tokens_next( &tokens );
					c = fgetc( infile );
					while ( c == ' ' || c == '\t' )
						c = fgetc( infile );
					if ( c == EOF )
						break;
					ungetc( c, infile );
				}
				// If we have 2 or more tokens, we add the space or tab to the current token,
				// but if it's a tab character, we add it regardless of the token count.
				else if ( c != ':' )
				{
					tokens_putc( &tokens, c );
				}
				break;
			default:
				// For any other character, if it's a printable character, we add it to the current token,
				// applying case conversion if specified in the options.
				c &= 0xFF; // Ensure we are working with a character in the range 0-255.
				if ( c >= ' ' )
				{
					if ( popts->ucase )
						c = toupper( c );
					else if ( popts->lcase )
						c = tolower( c );
					tokens_putc( &tokens, c );
				}
			}
		}
		lastc = c;
	}
	putc( 0x1A, outfile );
	puts( "End of file" );
}

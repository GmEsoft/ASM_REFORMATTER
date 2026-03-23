/***
 * ASM reformatter
 * Copyright (C) 2026 by GmEsoft
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * ASM reformatter: a simple tool to reformat assembly source code with various options.
 *
 * This tool reads an assembly source file, parses each line into tokens (label, mnemonic,
 * and then writes the reformatted lines to an output file with proper alignment and formatting
 * based on the specified options.
 *
 * The tool also includes an option to convert 8080 assembly code to Z-80 assembly code by
 * modifying the mnemonics and arguments accordingly.
 *
 * The main features of the tool include:
 * - Configurable tab size and alignment for labels, mnemonics, and comments.
 * - Options to convert mnemonics and register names to upper-case or lower-case,
 *   while preserving the case of literals and comments.
 * - Support for multi-statement lines with a configurable separator character.
 * - An option to align no-instruction comments with the mnemonics column.
 * - An option to convert 8080 assembly code to Z-80 assembly code by modifying the mnemonics
 *   and arguments accordingly.
 *
 * The tool is designed to be simple and efficient, with a focus on readability and maintainability
 * of the source code.
 *
 * The code is organized into several functions and structures to handle the parsing,
 * tokenization, and formatting of the assembly source code.
 *
 * The tool is intended for use by assembly language programmers who want to improve the
 * readability and consistency of their source code, or to convert existing 8080 assembly code
 * to Z-80 assembly code.
 *
 * Author: GmEsoft
 * Version: 0.2
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>

#include <fcntl.h>    // O_RDWR...
#include <sys/stat.h> // S_IWRITE

#define REFORMAT "ASM reformatter V0.2, (C) 2026 by GmEsoft"

#define TRACE 0
#define TRC if(TRACE)

typedef unsigned int uint32_t;
const int false = 0;
const int true = 1;

//=============================================================================
//		help
//=============================================================================
void help()
{
	puts(
		"REFORMAT -A|-C|-U|-X -I:infile [-O:outfile] [opts...]\n"
		"   -I:infile   Input image file\n"	 // infile
		"   -O:outfile  Output image file\n" // outfile
		"opts:\n"
		"   -C:n        Comments tab (default: 4)\n"							// comment_tab
		"   -L          Convert to lower-case (except literals and comments)\n" // lcase
		"   -M:n        Mnemonics tab (default: 2)\n"							// mnemo_tab
		"   -S:c        Multi-statements separator (default: '!')\n"			// sep
		"   -T:n        Tab size (default: 8)\n"								// tabsize
		"   -U          Convert to upper-case (except literals and comments)\n" // ucase
		"   -XZ         Convert 8080 code to Z-80 code\n"						// conv_z80
		"   -XN		    Do not align no-instruction comments with mnemonics\n"	// noinstr_comments
		"   -XS         Do not convert multi-statement separator in comments\n" // sep_in_comment
		"   -?          Show this help message\n"
		"Example:\n"
		"   reformat -U -T:4 -M:2 -N:4 -I:input.asm -O:output.asm\n");
}

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
} options_t;


//=============================================================================
//		str_t: a simple string class with fixed max length
//=============================================================================
typedef struct
{
	char	s[256];		// string buffer (max 255 chars + null terminator)
} str_t;

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
void str_putc( str_t *pStr, char c )
{
	int p = strlen( pStr->s );
	if ( p < 255 )
	{
		pStr->s[p++] = c;
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
void tokens_putc( tokens_t *pTokens, char c )
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
	int cmt = false;
	char quot = 0;
	char *cstr;

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
void tokens_write( tokens_t *pTokens, FILE *outfile, options_t *opts )
{
	int tab = 0;
	int pos = 0;
	int len = 0;
	int lastc = 0;
	int tabsize = opts->tabsize;
	int mnemo_tab = opts->mnemo_tab;
	int comment_tab = opts->comment_tab;
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
//		convert 8080 to Z-80
//=============================================================================

// The functions below convert 8080 register names and instructions to their Z-80 equivalents.

// Converts 8080 8-bit register names to Z-80 equivalents, handling the special case of "M".
char *convz80reg8( char *reg )
{
	if ( !stricmp( reg, "M" ) )
		return "(HL)";
	return reg;
}

// Converts 8080 16-bit register names to Z-80 equivalents, handling the special case of "PSW".
char *convz80reg16( char *reg )
{
	if ( !stricmp( reg, "B" ) )
		return "BC";
	if ( !stricmp( reg, "D" ) )
		return "DE";
	if ( !stricmp( reg, "H" ) )
		return "HL";
	if ( !stricmp( reg, "PSW" ) )
		return "AF";
	return reg;
}

// Converts an argument to a Z-80 memory reference by wrapping it in parentheses.
char *convz80at( char *arg )
{
	static char ret[256];
	strcpy( ret, "(" );
	strcat( ret, arg );
	strcat( ret, ")" );
	return ret;
}

// Converts 8080 instructions to their Z-80 equivalents, modifying the tokens in place.
void convz80( tokens_t *pTokens )
{
	tokens_t args;
	char *arg1, *arg2;

	// Note: the functions below assume that the tokens are well-formed
	// and do not perform extensive error checking.
	int count = tokens_count( pTokens );

	// We get the mnemonic and the first argument, if they exist, to determine if we need to convert.
	char *op = count < 1 ? "" : tokens_getcstr( pTokens, 1 );
	char *arg = count < 2 ? "" : tokens_getcstr( pTokens, 2 );

	// If there are no tokens or the mnemonic is not starting with an alphabetic character, we skip conversion.
	if ( !count || !isalpha(op[0]) )
		return;

	// We skip conversion for assembler directives and pseudo-ops, as they are not actual instructions.
	if	(	!stricmp( op, "DEFB" ) || !stricmp( op, "DB" )
	  	||	!stricmp( op, "DEFW" ) || !stricmp( op, "DW" )
	  	||	!stricmp( op, "DEFS" ) || !stricmp( op, "DS" )
	  	||	!stricmp( op, "EQU" ) || !stricmp( op, "SET" )
	  	||	!stricmp( op, "ORG" ) || !stricmp( op, "END" )
	  	||	!stricmp( op, "IF" ) || !stricmp( op, "ELSE" ) || !stricmp( op, "ENDIF" )
		)
	{
		return;
	}

	// We split the arguments into separate tokens for easier processing, as some instructions have multiple arguments.
	tokens_splitargs( &args, arg );

	arg1 = tokens_getcstr( &args, 0 );
	arg2 = args.ntokens < 1 ? "" : tokens_getcstr( &args, 1 );

	// We check the mnemonic against known 8080 instructions and convert them to Z-80 equivalents,
	// modifying the opcode and arguments as needed.
	if ( !stricmp ( op, "JMP" ) ) {			// JP	nnnn
		strcpy( op, "JP" );
	} else if ( !stricmp ( op, "JZ" ) ) {	// JP	Z,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "Z";
	} else if ( !stricmp ( op, "JNZ" ) ) {	// JP	NZ,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "NZ";
	} else if ( !stricmp ( op, "JC" ) ) {	// JP	C,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "C";
	} else if ( !stricmp ( op, "JNC" ) ) {	// JP	NC,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "NC";
	} else if ( !stricmp ( op, "JPO" ) ) {	// JP	PO,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "PO";
	} else if ( !stricmp ( op, "JPE" ) ) {	// JP	PE,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "PE";
	} else if ( !stricmp ( op, "JP" ) ) {	// JP	P,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "P";
	} else if ( !stricmp ( op, "JM" ) ) {	// JP	M,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "M";
	} else if ( !stricmp ( op, "PCHL" ) ) {	// JP	(HL)
		strcpy( op, "JP" );
		arg1 = "(HL)";
	} else if ( !stricmp ( op, "CALL" ) ) {	// CALL	nnnn
	} else if ( !stricmp ( op, "CZ" ) ) {	// CALL	Z,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "Z";
	} else if ( !stricmp ( op, "CNZ" ) ) {	// CALL	NZ,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "NZ";
	} else if ( !stricmp ( op, "CC" ) ) {	// CALL	C,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "C";
	} else if ( !stricmp ( op, "CNC" ) ) {	// CALL	NC,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "NC";
	} else if ( !stricmp ( op, "CPO" ) ) {	// CALL	PO,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "PO";
	} else if ( !stricmp ( op, "CPE" ) ) {	// CALL	PE,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "PE";
	} else if ( !stricmp ( op, "CP" ) ) {	// CALL	P,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "P";
	} else if ( !stricmp ( op, "CM" ) ) {	// CALL	M,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "M";
	} else if ( !stricmp ( op, "RET" ) ) {	// RET
	} else if ( !stricmp ( op, "RZ" ) ) {	// RET	Z
		strcpy( op, "RET" );
		arg1 = "Z";
	} else if ( !stricmp ( op, "RNZ" ) ) {	// RET	NZ
		strcpy( op, "RET" );
		arg1 = "NZ";
	} else if ( !stricmp ( op, "RC" ) ) {	// RET	C
		strcpy( op, "RET" );
		arg1 = "C";
	} else if ( !stricmp ( op, "RNC" ) ) {	// RET	NC
		strcpy( op, "RET" );
		arg1 = "NC";
	} else if ( !stricmp ( op, "RPO" ) ) {	// RET	PO
		strcpy( op, "RET" );
		arg1 = "PO";
	} else if ( !stricmp ( op, "RPE" ) ) {	// RET	PE
		strcpy( op, "RET" );
		arg1 = "PE";
	} else if ( !stricmp ( op, "RP" ) ) {	// RET	P
		strcpy( op, "RET" );
		arg1 = "P";
	} else if ( !stricmp ( op, "RM" ) ) {	// RET	M
		strcpy( op, "RET" );
		arg1 = "M";
	} else if ( !stricmp ( op, "RST" ) ) {	// RST	nn
		switch ( *arg1 ) {
		case '0': arg1 = "00H"; break;
		case '1': arg1 = "08H"; break;
		case '2': arg1 = "10H"; break;
		case '3': arg1 = "18H"; break;
		case '4': arg1 = "20H"; break;
		case '5': arg1 = "28H"; break;
		case '6': arg1 = "30H"; break;
		case '7': arg1 = "38H"; break;
		}
	} else if ( !stricmp ( op, "MOV" ) ) {	// LD	r,r
		strcpy( op, "LD" );
		arg1 = convz80reg8( arg1 );
		arg2 = convz80reg8( arg2 );
	} else if ( !stricmp ( op, "MVI" ) ) {	// LD	r,nn
		strcpy( op, "LD" );
		arg1 = convz80reg8( arg1 );
	} else if ( !stricmp ( op, "LXI" ) ) {	// LD	rr,nnnn
		strcpy( op, "LD" );
		arg1 = convz80reg16( arg1 );
	} else if ( !stricmp ( op, "LDA" ) ) {	// LD	A,(nnnn)
		strcpy( op, "LD" );
		arg2 = convz80at( arg1 );
		arg1 = "A";
	} else if ( !stricmp ( op, "STA" ) ) {	// LD	(nnnn),A
		strcpy( op, "LD" );
		arg1 = convz80at( arg1 );
		arg2 = "A";
	} else if ( !stricmp ( op, "LDAX" ) ) {	// LD	A,(rr)
		strcpy( op, "LD" );
		arg2 = convz80at( convz80reg16( arg1 ) );
		arg1 = "A";
	} else if ( !stricmp ( op, "STAX" ) ) {	// LD	(rr),A
		strcpy( op, "LD" );
		arg1 = convz80at( convz80reg16( arg1 ) );
		arg2 = "A";
	} else if ( !stricmp ( op, "LHLD" ) ) {	// LD	HL,(nnnn)
		strcpy( op, "LD" );
		arg2 = convz80at( arg1 );
		arg1 = "HL";
	} else if ( !stricmp ( op, "SHLD" ) ) {	// LD	(nnnn),HL
		strcpy( op, "LD" );
		arg1 = convz80at( arg1 );
		arg2 = "HL";
	} else if ( !stricmp ( op, "SPHL" ) ) {	// LD	SP,HL
		strcpy( op, "LD" );
		arg1 = "SP";
		arg2 = "HL";
	} else if ( !stricmp ( op, "XCHG" ) ) {	// EX	DE,HL
		strcpy( op, "EX" );
		arg1 = "DE";
		arg2 = "HL";
	} else if ( !stricmp ( op, "XTHL" ) ) {	// EX	(SP),HL
		strcpy( op, "EX" );
		arg1 = "(SP)";
		arg2 = "HL";
	} else if ( !stricmp ( op, "ADI" )		// ADD	A,nn
			|| !stricmp ( op, "ADD" ) ) {	// ADD	A,r
		strcpy( op, "ADD" );
		arg2 = convz80reg8( arg1 );
		arg1 = "A";
	} else if ( !stricmp ( op, "ACI" )		// ADC	A,nn
			|| !stricmp ( op, "ADC" ) ) {	// ADC	A,r
		strcpy( op, "ADC" );
		arg2 = convz80reg8( arg1 );
		arg1 = "A";
	} else if ( !stricmp ( op, "SUI" )		// SUB	nn
			|| !stricmp ( op, "SUB" ) ) {	// SUB	r
		strcpy( op, "SUB" );
		arg1 = convz80reg8( arg1 );
	} else if ( !stricmp ( op, "SBI" )		// SBC	A,nn
			|| !stricmp ( op, "SBB" ) ) {	// SBC	A,r
		strcpy( op, "SBC" );
		arg2 = convz80reg8( arg1 );
		arg1 = "A";
	} else if ( !stricmp ( op, "DAD" ) ) {	// ADD	HL,rr
		strcpy( op, "ADD" );
		arg2 = convz80reg16( arg1 );
		arg1 = "HL";
	} else if ( !stricmp ( op, "DI" ) 		// DI
			|| !stricmp ( op, "EI" ) 		// EI
			|| !stricmp ( op, "NOP" ) ) {	// NOP
	} else if ( !stricmp ( op, "HLT" ) ) {	// HALT
		strcpy( op, "HALT" );
	} else if ( !stricmp ( op, "INR" ) ) {	// INC	r
		strcpy( op, "INC" );
		arg1 = convz80reg8( arg1 );
	} else if ( !stricmp ( op, "DCR" ) ) {	// DEC	r
		strcpy( op, "DEC" );
		arg1 = convz80reg8( arg1 );
	} else if ( !stricmp ( op, "INX" ) ) {	// INC	rr
		strcpy( op, "INC" );
		arg1 = convz80reg16( arg1 );
	} else if ( !stricmp ( op, "DCX" ) ) {	// DEC	rr
		strcpy( op, "DEC" );
		arg1 = convz80reg16( arg1 );
	} else if ( !stricmp ( op, "DAA" ) ) {	// DAA
	} else if ( !stricmp ( op, "CMA" ) ) {	// CPL
		strcpy( op, "CPL" );
	} else if ( !stricmp ( op, "STC" ) ) {	// SCF
		strcpy( op, "SCF" );
	} else if ( !stricmp ( op, "CMC" ) ) {	// CCF
		strcpy( op, "CCF" );
	} else if ( !stricmp ( op, "RLC" ) ) {	// RLCA
		strcpy( op, "RLCA" );
	} else if ( !stricmp ( op, "RRC" ) ) {	// RRCA
		strcpy( op, "RRCA" );
	} else if ( !stricmp ( op, "RAL" ) ) {	// RLA
		strcpy( op, "RLA" );
	} else if ( !stricmp ( op, "RAR" ) ) {	// RRA
		strcpy( op, "RRA" );
	} else if ( !stricmp ( op, "ANI" ) 	// AND	nn
			|| !stricmp ( op, "ANA" ) ) {	// AND	r
		strcpy( op, "AND" );
		arg1 = convz80reg8( arg1 );
	} else if ( !stricmp ( op, "XRI" ) 	// XOR	nn
			|| !stricmp ( op, "XRA" ) ) {	// XOR	r
		strcpy( op, "XOR" );
		arg1 = convz80reg8( arg1 );
	} else if ( !stricmp ( op, "ORI" ) 	// OR	nn
			|| !stricmp ( op, "ORA" ) ) {	// OR	r
		strcpy( op, "OR" );
		arg1 = convz80reg8( arg1 );
	} else if ( !stricmp ( op, "CPI" ) 	// CP	nn
			|| !stricmp ( op, "CMP" ) ) {	// CP	r
		strcpy( op, "CP" );
		arg1 = convz80reg8( arg1 );
	} else if ( !stricmp ( op, "PUSH" ) 	// PUSH	rr
			|| !stricmp ( op, "POP" ) ) {	// POP	rr
		arg1 = convz80reg16( arg1 );
	} else if ( !stricmp ( op, "IN" ) ) {	// IN	A,(nn)
		arg2 = convz80at( arg1 );
		arg1 = "A";
	} else if ( !stricmp ( op, "OUT" ) ) { // OUT	(nn),A
		arg1 = convz80at( arg1 );
		arg2 = "A";
	} else {
		return;
	}

	// After determining the new opcode and arguments, we update the tokens with the converted values.
	if ( *arg1 )
	{
		if ( count < 2 )
		{
			tokens_next( pTokens );
			arg = tokens_getcstr( pTokens, 2 );
		}
		strcpy( arg, arg1 );
		if ( *arg2 )
		{
			strcat( arg, "," );
			strcat( arg, arg2 );
		}
	}
}

//=============================================================================
//		reformat: reads the input file character by character,
//		tokenizes it into labels, mnemonics, arguments, and comments,
//		applies formatting and optional 8080 to Z-80 conversion,
//		and writes the output file.
//=============================================================================
void reformat( FILE *infile, FILE *outfile, options_t *opts )
{
	int c;
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
			if ( c && opts->sep_in_comment && c == opts->sep )
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
				if ( opts->conv_z80 )
					convz80( &tokens );
				tokens_write( &tokens, outfile, opts );
				tokens_init( &tokens );
			}
		}
		// If we are not in a comment or a quoted string, we handle characters according
		// to their role in assembly syntax.
		else
		{
			// If 'sep' option is set and we encounter the separator character,
			// we treat it as a newline.
			if ( c && c == opts->sep )
			{
				c = 0x0A;
			}

			// We handle different characters based on their role in assembly syntax.
			switch ( c )
			{
			case 0x0D:
			case 0x0A: // Newline characters
				quote = cmt = 0;
				if ( opts->conv_z80 )
					convz80( &tokens );
				tokens_write( &tokens, outfile, opts );
				tokens_init( &tokens );
				break;
			case ';': // Comment character
				cmt = 1;
				tokens_set( &tokens,
					  tokens_empty( &tokens ) ? 0
					: opts->noinstr_comments && tokens_count( &tokens ) < 2 && !tokens_getcstr( &tokens, 1 )[0] ? 1
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
					//*tokens_get( &tokens, 0 ) = *tokens_get( &tokens, 1 );
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
				if ( c >= ' ' )
				{
					if ( opts->ucase )
						c = toupper( c );
					else if ( opts->lcase )
						c = tolower( c );
					tokens_putc( &tokens, c );
				}
			}
		}
	}
	puts( "End of file" );
}

//=============================================================================
//		main: parses command-line arguments, opens files, and calls reformat.
//=============================================================================
int main( int argc, char* argv[] )
{
	FILE *infile=0, *outfile=0;

	int	i;

	char sub = 0;

	puts( REFORMAT "\n" );

	options_t opts = {
		.sep = 0,
		.tabsize = 8,
		.comment_tab = 4,
		.mnemo_tab = 1,
		.ucase = 0,
		.lcase = 0,
		.sep_in_comment = 0,
		.noinstr_comments = 0,
		.conv_z80 = 0
	};

	for ( i=1; i<argc; ++i )
	{
		char *s = argv[i];
		char c = 0;

		if ( *s == '-' )
			++s;
		switch ( toupper( *s ) )
		{
		case 'I': // Input file
			++s;
			if ( *s == ':' )
				++s;
			printf( "Reading: %s\n", s );
			infile = fopen( s, "r" );
			break;
		case 'O': // Output file
			++s;
			if ( *s == ':' )
				++s;
			printf( "Creating: %s\n", s );
			outfile = fopen( s, "w" );
			break;
		case 'C': // Comments tab
			++s;
			if ( *s == ':' )
				++s;
			opts.comment_tab = atoi( s );
			//printf( "opts.comment_tab=%d\n", opts.comment_tab );
			break;
		case 'L': // Lower-case
			++s;
			opts.lcase = 1;
			break;
		case 'M': // Mnemonics tab
			++s;
			if ( *s == ':' )
				++s;
			opts.mnemo_tab = atoi( s );
			break;
		case 'S': // Separator
			++s;
			if ( *s == ':' )
				++s;
			opts.sep = *s;
			break;
		case 'T': // Tab size
			++s;
			if ( *s == ':' )
				++s;
			opts.tabsize = atoi( s );
			break;
		case 'U': // Upper-case
			++s;
			opts.ucase = 1;
			break;
		case 'X': // Extra options
			++s;
			if ( toupper( *s ) == 'S' ) // Allow separator in comments
				opts.sep_in_comment = 1;
			else if ( toupper( *s ) == 'N' ) // Align No-instr comments with mnemonics
				opts.noinstr_comments = 1;
			else if ( toupper( *s ) == 'Z' ) // Convert 8080 code to Z-80
				opts.conv_z80 = 1;
			break;
		case '?': // Help
			help();
			return 0;
		default: // Unrecognized switch
			printf( "Unrecognized switch: -%s\n", s );
			printf( "REFORMAT -? for help.\n" );
			return 1;
		}

		if ( errno )
		{
			puts( strerror( errno ) );
			return 1;
		}
	}

	if ( !infile )
	{
		puts( "Missing -I:infile" );
		return 1;
	}

	if ( !outfile )
	{
		puts( "Missing -O:outfile" );
		return 1;
	}

	reformat( infile, outfile, &opts );

	fclose( infile );

	fclose( outfile );

	return 0;
}

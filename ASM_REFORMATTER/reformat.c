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
 * Version: 0.4
 *
 */

#define _CRT_SECURE_NO_WARNINGS

#include "reformatter.h"
#include "convz80.h"
#include "conv8080.h"

#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include <fcntl.h>    // O_RDWR...
#include <sys/stat.h> // S_IWRITE

#define REFORMAT "ASM reformatter V0.6, (C) 2026 by GmEsoft"



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
		"   -XE         Add EOF char at end of file\n"							// eof
		"   -XN         Align no-instruction comments with mnemonics\n"			// noinstr_comments
		"   -XS         Convert multi-statement separator in comments\n"		// sep_in_comment
		"   -XZ         Convert 8080 code to Z-80 code\n"						// conv_z80
		"   -X8         Convert Z-80 code to 8080 code\n"						// conv_8080
		"   -XL         Convert 8080 code from/to Z-80 using z80lib macros\n"	// conv_z80lib
		"   -X*         Mark unrecognized instructions\n" 						// mark_unknown
		"   -?          Show this help message\n"
		"Example:\n"
		"   reformat -U -T:4 -M:2 -N:4 -I:input.asm -O:output.asm\n" );
}




//=============================================================================
//		main: parses command-line arguments, opens files, and calls reformat.
//=============================================================================
int main( int argc, char* argv[] )
{
	FILE *infile=0, *outfile=0;

	int	i;

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
		.conv_z80 = 0,
		.conv_8080 = 0,
		.conv_z80lib = 0,
		.mark_unknown = 0,
		.eof = 0
	};

	for ( i=1; i<argc; ++i )
	{
		char *s = argv[i];
//		char c = 0;

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
			if ( *s )
				opts.sep = *s;
			else
				opts.sep = '!'; // Default separator character if none provided
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
			if ( toupper( *s ) == 'E' ) // Add EOF char at end of file
				opts.eof = 1;
			else if ( toupper( *s ) == 'S' ) // Allow separator in comments
				opts.sep_in_comment = 1;
			else if ( toupper( *s ) == 'N' ) // Align No-instr comments with mnemonics
				opts.noinstr_comments = 1;
			else if ( toupper( *s ) == 'Z' ) // Convert 8080 code to Z-80
				opts.conv_z80 = 1;
			else if ( toupper( *s ) == '8' ) // Convert Z-80 code to 8080
				opts.conv_8080 = 1;
			else if ( toupper( *s ) == 'L' ) // Use Z80Lib macro definitions
				opts.conv_z80lib = 1;
			else if ( toupper( *s ) == '*' ) // Mark unrecognized instructions
				opts.mark_unknown = 1;
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
			char buf[256];
			strerror_s( buf, sizeof( buf ), errno );
			puts( buf );
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

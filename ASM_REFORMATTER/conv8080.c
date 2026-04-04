#define _CRT_SECURE_NO_WARNINGS

#include "convz80.h"

#include <string.h>
#include <ctype.h>
#include <stdlib.h> // for strtol

//=============================================================================
//		convert Z-80 to 8080
//=============================================================================



// The functions below convert Z-80 register names and instructions to their 8080 equivalents.

// parse a decimal or hexadecimal number from a string, and return its value as an integer.
// The function handles both decimal numbers (e.g., "123") and hexadecimal numbers (e.g.,  "7Bh"), and returns the corresponding integer value. If the string is not a valid number, the function returns -1.
static int parse_int( char *str )
{
	char *endptr;
	int base = 10;
	if ( str[0] == '0' && ( str[1] == 'x' || str[1] == 'X' ) ) {
		str += 2;
		base = 0x10;
	} else if ( str[strlen( str ) - 1] == 'h' || str[strlen( str ) - 1] == 'H' ) {
		str[strlen( str ) - 1] = 0;
		base = 0x10;
	}
	return (int)strtol( str, &endptr, base );
}

// Converts Z-80 8-bit register names to 8080 equivalents, handling the special case of "M".
static char *conv8080reg8( char *reg )
{
	if ( !_strcmpi( reg, "(HL)" ) )
		return "M";
	return reg;
}

// Converts Z-80 memory references to 8080 equivalents by removing the parentheses and index.
static char *conv8080at( char *reg )
{
	static char ret[256];
	if ( reg[0] == '(' && reg[strlen( reg ) - 1] == ')' ) {
		// We assume that the argument is a simple memory reference like (nnnn) or (IX+offset), and we convert it to 8080 format by removing the parentheses and index.
		// 1. Handle the case of (IX+offset) or (IY+offset) by stripping the index and keeping only the offset, as 8080 does not support indexed addressing.
		if ( reg[1] == 'I' && ( reg[2] == 'X' || reg[2] == 'Y' ) && ( reg[3] == '+' || reg[3] == '-' || reg[3] == ')' ) ) {
			switch ( reg[3] ) {
			case '+':
				// We copy the offset part of the string except the plus sign, which starts after "IX+" or "IY+" and ends before the closing parenthesis.
				strncpy( ret, reg + 4, strlen( reg ) - 5 );
				ret[strlen( reg ) - 5] = 0;
				break;
			case '-':
				// We copy the offset part of the string including the minus sign, which starts after "IX" or "IY" and ends before the closing parenthesis.
				strncpy( ret, reg + 3, strlen( reg ) - 4 );
				ret[strlen( reg ) - 4] = 0;
				break;
			case ')':
				// If there is no offset, we just return "0" as the memory reference.
				strcpy( ret, "0" );
				break;
			}
		} else
		// 2. Handle the case of (nnnn) by stripping the parentheses.
		{
			strncpy( ret, reg + 1, strlen( reg ) - 2 );
			ret[strlen( reg ) - 2] = 0;
		}
		return ret;
	}
	return reg;
}

// Converts Z-80 16-bit register names to 8080 equivalents, handling the special case of "PSW".
static char *conv8080reg16( char *reg )
{
	if ( !_strcmpi( reg, "BC" ) )
		return "B";
	if ( !_strcmpi( reg, "DE" ) )
		return "D";
	if ( !_strcmpi( reg, "HL" ) )
		return "H";
	if ( !_strcmpi( reg, "AF" ) )
		return "PSW";
	if ( !_strcmpi( reg, "SP" ) )
		return "SP";
	return reg;
}

// test if the argument is a 8-bit register or a memory reference via (HL)
static int is_reg8( char *arg )
{
	return !_strcmpi( arg, "A" ) ||
		   !_strcmpi( arg, "B" ) ||
		   !_strcmpi( arg, "C" ) ||
		   !_strcmpi( arg, "D" ) ||
		   !_strcmpi( arg, "E" ) ||
		   !_strcmpi( arg, "H" ) ||
		   !_strcmpi( arg, "L" ) ||
		   !_strcmpi( arg, "(HL)" );
}

// test if the argument is the 8080 accumulator register A
static int is_acc( char *arg )
{
	return !_strcmpi( arg, "A" );
}

// test if the argument is the Z-80 register I or R, which have no 8080 equivalent
static int is_ir( char *arg )
{
	return !_strcmpi( arg, "I" ) || !_strcmpi( arg, "R" );
}

// test if the argument is the 8080 memory pointer HL
static int is_hl( char *arg )
{
	return !_strcmpi( arg, "HL" );
}

// test if the argument is the Z-80 index register IX or IY, which have no 8080 equivalent
static int is_ixiy( char *arg )
{
	return !_strcmpi( arg, "IX" ) || !_strcmpi( arg, "IY" );
}

// test if the argument is the 8080 register pair DE
static int is_de( char *arg )
{
	return !_strcmpi( arg, "DE" );
}

// test if the argument is the 8080 stack pointer register SP
static int is_sp( char *arg )
{
	return !_strcmpi( arg, "SP" );
}

// test if the argument is a 16-bit register
static int is_reg16( char *arg )
{
	return !_strcmpi( arg, "BC" ) ||
		   !_strcmpi( arg, "DE" ) ||
		   !_strcmpi( arg, "HL" ) ||
		   !_strcmpi( arg, "SP" );
}

// test if the argument is a 8080 16-bit register pair (BC, DE, HL, AF)
static int is_regpair( char *arg )
{
	return !_strcmpi( arg, "BC" ) ||
		   !_strcmpi( arg, "DE" ) ||
		   !_strcmpi( arg, "HL" ) ||
		   !_strcmpi( arg, "AF" );
}

// test if the argument is a 8080 direct memory reference excluding (BC), (DE), (HL) (IX+offset) or (IY+offset)
int is_direct( char *arg )
{
	return arg[0] == '(' && arg[strlen( arg ) - 1] == ')' &&
		   !_strcmpi( arg, "(BC)" ) == 0 &&
		   !_strcmpi( arg, "(DE)" ) == 0 &&
		   !_strcmpi( arg, "(HL)" ) == 0 &&
		   ( ( toupper( arg[1] ) == 'I' && ( toupper( arg[2] ) == 'X' || toupper( arg[2] ) == 'Y' ) )
				   ? ( arg[3] != '+' && arg[3] != '-' && arg[3] != ')' )
				   : 1 );
}

int is_immediate( char *arg )
{
	return ( arg[0] != '(' || arg[strlen( arg ) - 1] != ')' ) &&
		   !is_reg8( arg ) && !is_reg16( arg );
}

// test if the argument is a 8080 condition code (Z, NZ, C, NC, PO, PE, P, M)
int is_cond( char *arg )
{
	return !_strcmpi( arg, "Z" ) ||
		   !_strcmpi( arg, "NZ" ) ||
		   !_strcmpi( arg, "C" ) ||
		   !_strcmpi( arg, "NC" ) ||
		   !_strcmpi( arg, "PO" ) ||
		   !_strcmpi( arg, "PE" ) ||
		   !_strcmpi( arg, "P" ) ||
		   !_strcmpi( arg, "M" );
}

// test if the argument is a 8-bit 8080 indirect memory reference via (BC) or (DE)
int is_8080_indirect( char *arg )
{
	return !_strcmpi( arg, "(BC)" ) || !_strcmpi( arg, "(DE)" );
}

// test if the argument is a 8-bit Z-80 indirect memory reference via (BC) or (DE)
int is_Z80_indirect8( char *arg )
{
	return !_strcmpi( arg, "(BC)" ) ||
		   !_strcmpi( arg, "(DE)" );
}

// test if the argument is a 8-bit Z-80 indexed memory reference via (IX+offset) or (IY+offset)
int is_Z80_indexed( char *arg )
{
	return ( arg[0] == '(' && arg[strlen( arg ) - 1] == ')' &&
			 toupper( arg[1] ) == 'I' && ( toupper( arg[2] ) == 'X' || toupper( arg[2] ) == 'Y' ) );
}

// test if the argument is a 16-bit Z-80 indirect memory reference via (SP)
int is_Z80_indirect16( char *arg )
{
	return !_strcmpi( arg, "(SP)" );
}

// Converts Z-80 instructions to their 8080 equivalents, modifying the tokens in place.
void conv8080( tokens_t *pTokens, options_t *popts )
{
	tokens_t args;
	char *arg1, *arg2;
	int z80lib = popts->conv_z80lib;

	// Note: the functions below assume that the tokens are well-formed
	// and do not perform extensive error checking.
	int count = tokens_count( pTokens );

	// We get the mnemonic and the first argument, if they exist, to determine if we need to convert.
	char *op = count < 1 ? "" : tokens_getcstr( pTokens, 1 );
	char *arg = count < 2 ? "" : tokens_getcstr( pTokens, 2 );

	// If there are no tokens or the mnemonic is not starting with an alphabetic character, we skip conversion.
	if ( !count || !isalpha( op[0] ) )
		return;

	// We skip conversion for assembler directives and pseudo-ops, as they are not actual instructions.
	if ( !_strcmpi( op, "DEFB" ) || !_strcmpi( op, "DB" ) ||
		 !_strcmpi( op, "DEFW" ) || !_strcmpi( op, "DW" ) ||
		 !_strcmpi( op, "DEFS" ) || !_strcmpi( op, "DS" ) ||
		 !_strcmpi( op, "EQU" ) ||
		 !_strcmpi( op, "PUBLIC" ) || !_strcmpi( op, "EXTRN" ) ||
		 !_strcmpi( op, "TITLE" ) || !_strcmpi( op, "MACLIB" ) ||
		 !_strcmpi( op, "ASEG" ) || !_strcmpi( op, "CSEG" ) || !_strcmpi( op, "DSEG" ) ||
		 !_strcmpi( op, "ORG" ) || !_strcmpi( op, "END" ) ||
		 !_strcmpi( op, "IF" ) || !_strcmpi( op, "ELSE" ) || !_strcmpi( op, "ENDIF" ) ) {
		return;
	}

	// We split the arguments into separate tokens for easier processing, as some instructions have multiple arguments.
	tokens_splitargs( &args, arg );

	arg1 = tokens_getcstr( &args, 0 );
	arg2 = args.ntokens < 1 ? "" : tokens_getcstr( &args, 1 );

	// We check the mnemonic against known 8080 instructions and convert them to Z-80 equivalents,
	// modifying the opcode and arguments as needed.
	if ( !_strcmpi( op, "JP" ) ) { // JP	nnnn
		if ( !_strcmpi( arg1, "(HL)" ) ) {
			strcpy( op, "PCHL" );
			arg1[0] = 0;
		} else if ( z80lib && !_strcmpi( arg1, "(IX)" ) ) {
			strcpy( op, "PCIX" );
			arg1[0] = 0;
		} else if ( z80lib && !_strcmpi( arg1, "(IY)" ) ) {
			strcpy( op, "PCIY" );
			arg1[0] = 0;
		} else if ( is_cond( arg1 ) && arg2[0] ) {
			strcpy( op, "J" );
			strcat( op, arg1 );
			arg1 = arg2;
			arg2 = "";
		} else {
			strcpy( op, "JMP" );
		}
	} else if ( !_strcmpi( op, "CALL" ) ) { // CALL	nnnn
		if ( is_cond( arg1 ) && arg2[0] ) {
			strcpy( op, "C" );
			strcat( op, arg1 );
			arg1 = arg2;
			arg2 = "";
		} else {
			strcpy( op, "CALL" );
		}
	} else if ( !_strcmpi( op, "RET" ) ) { // RET
		if ( is_cond( arg1 ) ) {
			strcpy( op, "R" );
			strcat( op, arg1 );
			arg1 = arg2 = "";
		} else {
			strcpy( op, "RET" );
		}
	} else if ( !_strcmpi( op, "RST" ) ) { // RST	nn
		int rst_num = parse_int( arg1 );
		// printf( "Parsed RST number: %d\n", rst_num ); // Debug output to verify correct parsing
		if ( rst_num >= 0x00 && rst_num <= 0x38 && ( rst_num & 7 ) == 0 ) {
			static char rst_str[4] = "0";
			arg1 = rst_str;
			strcpy( op, "RST" );
			arg1[0] = '0' + (char)( rst_num / 8 ); // Convert 0,8,16,...,56 to '0','1','2',...,'7'
			arg2 = "";
		} else {
			strcat( op, "?" ); // can't convert now
			return;
		}
	} else if ( !_strcmpi( op, "LD" ) ) {
		if ( is_reg8( arg1 ) && is_reg8( arg2 ) ) { // LD	r,r
			strcpy( op, "MOV" );
			arg1 = conv8080reg8( arg1 );
			arg2 = conv8080reg8( arg2 );
		} else if ( z80lib && is_reg8( arg1 ) && is_Z80_indexed( arg2 ) ) { // LD	r,(IX|IY+offset)
			strcpy( op, "LDX" );
			op[2] = (char)toupper( arg2[2] ); // LDX or LDY
			arg2 = conv8080at( arg2 );
		} else if ( z80lib && is_Z80_indexed( arg1 ) && is_reg8( arg2 ) ) { // LD	(IX|IY+offset),r
			char *t = conv8080at( arg1 );
			strcpy( op, "STX" );
			op[2] = (char)toupper( arg1[2] ); // STX or STY
			arg1 = arg2;
			arg2 = t;
		} else if ( z80lib && is_ir( arg1 ) && is_Z80_indexed( arg2 ) ) { // LD	I,A or LD R,A
			strcpy( op, "STA" );
			op[3] = (char)toupper( arg2[2] ); // STAI or STAR
			arg1 = arg2 = "";
		} else if ( z80lib && is_acc( arg1 ) && is_ir( arg2 ) ) { // LD	A,I or LD A,R
			strcpy( op, "LDA" );
			strcat( op, arg2 ); // LDAI or LDAR
			arg1 = arg2 = "";
		} else if ( z80lib && is_ir( arg1 ) && is_acc( arg2 ) ) { // LD	I,A or LD R,A
			strcpy( op, "STA" );
			strcat( op, arg1 ); // STAI or STAR
			arg1 = arg2 = "";
		} else if ( is_acc( arg1 ) && is_8080_indirect( arg2 ) ) { // LD	A,(rr)
			strcpy( op, "LDAX" );
			arg1 = conv8080reg16( conv8080at( arg2 ) );
			arg2 = "";
		} else if ( is_8080_indirect( arg1 ) && is_acc( arg2 ) ) { // LD	(rr),A
			strcpy( op, "STAX" );
			arg1 = conv8080reg16( conv8080at( arg1 ) );
			arg2 = "";
		} else if ( is_acc( arg1 ) && is_direct( arg2 ) ) { // LD	A,(nnnn)
			strcpy( op, "LDA" );
			arg1 = conv8080at( arg2 );
			arg2 = "";
		} else if ( is_direct( arg1 ) && is_acc( arg2 ) ) { // LD	(nnnn),A
			strcpy( op, "STA" );
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else if ( is_hl( arg1 ) && is_direct( arg2 ) ) { // LD	HL,(nnnn)
			strcpy( op, "LHLD" );
			arg1 = conv8080at( arg2 );
			arg2 = "";
		} else if ( z80lib && ( is_ixiy( arg1 ) || is_reg16( arg1 ) ) && is_direct( arg2 ) ) { // LD IX|IY|BC|DE|SP,(nnnn)
			strcpy( op, "L" );
			strcat( op, arg1 ); // LBCD, LDED, LIXD, LIYD
			strcat( op, "D" );
			arg1 = conv8080at( arg2 );
			arg2 = "";
		} else if ( z80lib && is_direct( arg1 ) && ( is_ixiy( arg2 ) || is_reg16( arg2 ) ) ) { // LD (nnnn),IX|IY|BC|DE|SP
			strcpy( op, "S" );
			strcat( op, arg2 ); // SBCD, SDED, SIXD, SIYD
			strcat( op, "D" );
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else if ( is_direct( arg1 ) && is_hl( arg2 ) ) { // LD	(nnnn),HL
			strcpy( op, "SHLD" );
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else if ( is_sp( arg1 ) && is_hl( arg2 ) ) { // LD	SP,HL
			strcpy( op, "SPHL" );
			arg1 = arg2 = "";
		} else if ( is_sp( arg1 ) && is_ixiy( arg2 ) ) { // LD	SP,IX|IY
			strcpy( op, "SP" );
			strcat( op, arg2 ); // SPIX or SPIY
			arg1 = arg2 = "";
		} else if ( is_reg8( arg1 ) && is_immediate( arg2 ) ) { // LD r,nn
			strcpy( op, "MVI" );
			arg1 = conv8080reg8( arg1 );
		} else if ( z80lib && is_Z80_indexed( arg1 ) && is_immediate( arg2 ) ) { // LD r,nn
			char *t = conv8080at( arg1 );
			strcpy( op, "MVIX" );
			op[3] = (char)toupper( arg1[2] ); // MVIX or MVIY
			arg1 = arg2;
			arg2 = t;
		} else if ( is_reg16( arg1 ) && is_immediate( arg2 ) ) { // LD rr,nnnn
			strcpy( op, "LXI" );
			arg1 = conv8080reg16( arg1 );
		} else if ( z80lib && is_ixiy( arg1 ) && is_immediate( arg2 ) ) { // LD IX|IY,nnnn
			strcpy( op, "LX" );
			strcat( op, arg1 ); // LXIX or LXIY
			arg1 = conv8080reg16( arg2 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized LD format, skip conversion.
		}
	} else if ( !_strcmpi( op, "EX" ) ) {		// EX ...
		if ( is_de( arg1 ) && is_hl( arg2 ) ) { // EX DE,HL
			strcpy( op, "XCHG" );
			arg1 = arg2 = "";
		} else if ( !_strcmpi( arg1, "(SP)" ) && is_hl( arg2 ) ) { // EX (SP),HL
			strcpy( op, "XTHL" );
			arg1 = arg2 = "";
		} else if ( z80lib && !_strcmpi( arg1, "(SP)" ) && is_ixiy( arg2 ) ) { // EX (SP),IX|IY
			strcpy( op, "XT" );
			strcat( op, arg2 ); // XTIX or XTIY
			arg1 = arg2 = "";
		} else if ( z80lib && !_strcmpi( arg1, "AF" ) && !_strcmpi( arg2, "AF'" ) ) { // EX AF,AF'
			strcpy( op, "EXAF" );
			arg1 = arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized EX format, skip conversion.
		}
	} else if ( !_strcmpi( op, "ADD" ) ||		   // ADD A,... - ADD HL|IX|IY,rr
				!_strcmpi( op, "ADC" ) ) {		   // ADC A,... - ADC HL|IX|IY,rr
		if ( is_acc( arg1 ) && is_reg8( arg2 ) ) { // ADD|ADC A,r
			arg1 = conv8080reg8( arg2 );
			arg2 = "";
		} else if ( is_acc( arg1 ) && is_immediate( arg2 ) ) { // ADI|ACI A,nn
			strcpy( op, !_strcmpi( op, "ADD" ) ? "ADI" : "ACI" );
			arg1 = arg2;
			arg2 = "";
		} else if ( z80lib && is_acc( arg1 ) && is_Z80_indexed( arg2 ) ) { // ADI|ACI A,(IX|IY+offset)
			strcpy( op, !_strcmpi( op, "ADD" ) ? "ADDX" : "ADCX" );
			op[3] = (char)toupper( arg2[2] ); // ADIX or ADCX
			arg1 = conv8080at( arg2 );
			arg2 = "";
		} else if ( is_hl( arg1 ) && is_reg16( arg2 ) && !_strcmpi( op, "ADD" ) ) { // ADD HL,rr
			strcpy( op, "DAD" );
			arg1 = conv8080reg16( arg2 );
			arg2 = "";
		} else if ( z80lib && is_ixiy( arg1 ) && ( is_ixiy( arg2 ) || is_reg16( arg2 ) ) && !_strcmpi( op, "ADD" ) ) { // ADD IX|IY,rr
			strcpy( op, "DADX" );
			op[3] = (char)toupper( arg1[1] ); // DADX or DADY
			arg1 = conv8080reg16( arg2 );
			arg2 = "";
		} else if ( z80lib && is_reg16( arg1 ) && is_reg16( arg2 ) && !_strcmpi( op, "ADC" ) ) { // ADC HL,r
			strcpy( op, "DADC" );
			arg1 = conv8080reg16( arg2 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized ADD format, skip conversion.
		}
	} else if ( !_strcmpi( op, "SUB" ) ) { // SUB ...
		if ( is_reg8( arg1 ) ) {		   // SUB r
			arg1 = conv8080reg8( arg1 );
			arg2 = "";
		} else if ( is_immediate( arg1 ) ) { // SUB nn
			strcpy( op, "SUI" );
		} else if ( z80lib && is_Z80_indexed( arg1 ) ) { // SUB (IX|IY+offset)
			strcpy( op, "SUBX" );
			op[3] = (char)toupper( arg1[2] ); // SUBX or SUBY
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized SUB format, skip conversion.
		}
	} else if ( !_strcmpi( op, "SBC" ) ) { // SBC A,... - SBC HL|IX|IY,rr
		if ( is_reg8( arg2 ) ) {		   // SBC A,r
			strcpy( op, "SBB" );
			arg1 = conv8080reg8( arg2 );
			arg2 = "";
		} else if ( is_immediate( arg2 ) ) { // SBC A,nn
			strcpy( op, "SBI" );
			arg1 = arg2;
			arg2 = "";
		} else if ( z80lib && is_acc( arg1 ) && is_Z80_indexed( arg2 ) ) { // SBC A,(IX|IY+offset)
			strcpy( op, "SBCX" );
			op[3] = (char)toupper( arg2[2] ); // SBCX or SBCY
			arg1 = conv8080at( arg2 );
			arg2 = "";
		} else if ( z80lib && is_reg16( arg1 ) && is_reg16( arg2 ) ) { // SBC HL,rr
			strcpy( op, "DSBC" );
			arg1 = conv8080reg16( arg2 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized SBC format, skip conversion.
		}
	} else if ( !_strcmpi( op, "DI" )		  // DI
				|| !_strcmpi( op, "EI" )	  // EI
				|| !_strcmpi( op, "NOP" ) ) { // NOP
	} else if ( !_strcmpi( op, "HALT" ) ) {	  // HALT
		strcpy( op, "HLT" );
	} else if ( !_strcmpi( op, "INC" ) ) { // INC
		if ( is_reg8( arg1 ) ) {		   // INC r
			strcpy( op, "INR" );
			arg1 = conv8080reg8( arg1 );
			arg2 = "";
		} else if ( z80lib && is_Z80_indexed( arg1 ) ) { // INC (IX|IY+offset)
			strcpy( op, "INRX" );
			op[3] = (char)toupper( arg1[2] ); // INRX or INRY
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else if ( is_reg16( arg1 ) ) { // INC rr
			strcpy( op, "INX" );
			arg1 = conv8080reg16( arg1 );
			arg2 = "";
		} else if ( z80lib && is_ixiy( arg1 ) ) { // INC IX|IY
			strcpy( op, "INX" );
			strcat( op, arg1 ); // INXIX or INXIY
			arg1 = conv8080reg16( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized INC format, skip conversion.
		}
	} else if ( !_strcmpi( op, "DEC" ) ) { // DEC ...
		if ( is_reg8( arg1 ) ) {		   // DEC r
			strcpy( op, "DCR" );
			arg1 = conv8080reg8( arg1 );
			arg2 = "";
		} else if ( z80lib && is_Z80_indexed( arg1 ) ) { // DEC (IX|IY+offset)
			strcpy( op, "DCRX" );
			op[3] = (char)toupper( arg1[2] ); // DCRX or DCRY
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else if ( is_reg16( arg1 ) ) { // DEC rr
			strcpy( op, "DCX" );
			arg1 = conv8080reg16( arg1 );
			arg2 = "";
		} else if ( z80lib && is_ixiy( arg1 ) ) { // DEC IX|IY
			strcpy( op, "DCX" );
			strcat( op, arg1 ); // DCXIX or DCXIY
			arg1 = conv8080reg16( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized DEC format, skip conversion.
		}
	} else if ( !_strcmpi( op, "DAA" ) ) { // DAA
	} else if ( !_strcmpi( op, "CPL" ) ) { // CPL
		strcpy( op, "CMA" );
	} else if ( !_strcmpi( op, "SCF" ) ) { // SCF
		strcpy( op, "STC" );
	} else if ( !_strcmpi( op, "CCF" ) ) { // CCF
		strcpy( op, "CMC" );
	} else if ( !_strcmpi( op, "RLCA" ) ) { // RLCA
		strcpy( op, "RLC" );
	} else if ( !_strcmpi( op, "RRCA" ) ) { // RRCA
		strcpy( op, "RRC" );
	} else if ( !_strcmpi( op, "RLA" ) ) { // RLA
		strcpy( op, "RAL" );
	} else if ( !_strcmpi( op, "RRA" ) ) { // RRA
		strcpy( op, "RAR" );
	} else if ( !_strcmpi( op, "AND" ) ) { // AND ...
		if ( is_reg8( arg1 ) ) {		   // AND r
			strcpy( op, "ANA" );
			arg1 = conv8080reg8( arg1 );
			arg2 = "";
		} else if ( is_immediate( arg1 ) ) { // AND nn
			strcpy( op, "ANI" );
		} else if ( z80lib && is_Z80_indexed( arg1 ) ) { // AND (IX|IY+offset)
			strcpy( op, "ANDX" );
			op[3] = (char)toupper( arg1[2] ); // ANDX or ANDY
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized AND format, skip conversion.
		}
	} else if ( !_strcmpi( op, "XOR" ) ) { // XOR ...
		if ( is_reg8( arg1 ) ) {		   // XOR r
			strcpy( op, "XRA" );
			arg1 = conv8080reg8( arg1 );
			arg2 = "";
		} else if ( is_immediate( arg1 ) ) { // XOR nn
			strcpy( op, "XRI" );
		} else if ( z80lib && is_Z80_indexed( arg1 ) ) { // XOR (IX|IY+offset)
			strcpy( op, "XORX" );
			op[3] = (char)toupper( arg1[2] ); // XORX or XORY
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized XOR format, skip conversion.
		}
	} else if ( !_strcmpi( op, "OR" ) ) { // OR ...
		if ( is_reg8( arg1 ) ) {		  // OR r
			strcpy( op, "ORA" );
			arg1 = conv8080reg8( arg1 );
			arg2 = "";
		} else if ( is_immediate( arg1 ) ) { // OR nn
			strcpy( op, "ORI" );
		} else if ( z80lib && is_Z80_indexed( arg1 ) ) { // OR (IX|IY+offset)
			strcpy( op, "ORX" );
			op[2] = (char)toupper( arg1[2] ); // ORX or ORY
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized OR format, skip conversion.
		}
	} else if ( !_strcmpi( op, "CP" ) ) { // CP ...
		if ( is_reg8( arg1 ) ) {		  // CP r
			strcpy( op, "CMP" );
			arg1 = conv8080reg8( arg1 );
			arg2 = "";
		} else if ( is_immediate( arg1 ) ) { // CP nn
			strcpy( op, "CPI" );
		} else if ( z80lib && is_Z80_indexed( arg1 ) ) { // CP (IX|IY+offset)
			strcpy( op, "CMPX" );
			op[3] = (char)toupper( arg1[2] ); // CPX or CPY
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized CP format, skip conversion.
		}
	} else if ( !_strcmpi( op, "PUSH" )		  // PUSH	rr
				|| !_strcmpi( op, "POP" ) ) { // POP	rr
		if ( is_regpair( arg1 ) ) {
			arg1 = conv8080reg16( arg1 );
		} else if ( z80lib && is_ixiy( arg1 ) ) {
			strcat( op, arg1 ); // PUSHIX or POPIX, PUSHIY or POPIY
			arg1 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized PUSH/POP format, skip conversion.
		}
	} else if ( !_strcmpi( op, "IN" ) ) {			 // IN A,(nn) - IN r,(C)
		if ( z80lib && is_reg8( arg1 ) && !_strcmpi( arg2, "(C)" ) ) { // IN r,(C)
			strcpy( op, "INP" );
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else if ( is_acc( arg1 ) && is_direct( arg2 ) ) { // IN A,(nn)
			arg1 = conv8080at( arg2 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized IN format, skip conversion.
		}
	} else if ( !_strcmpi( op, "OUT" ) ) { // OUT	(nn),A - OUT (C),r
		if ( z80lib && !_strcmpi( arg1, "(C)" ) && is_reg8( arg2 ) ) { // OUT (C),r
			strcpy( op, "OUTP" );
			arg1 = conv8080at( arg2 );
			arg2 = "";
		} else if ( is_acc( arg2 ) && is_direct( arg1 ) ) {
			strcpy( op, "OUT" ); // OUT (nn),A
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized OUT format, skip conversion.
		}
	} else if ( !z80lib ) {
		// If the instruction is not recognized and we're not converting Z80.LIB macros, we skip conversion.
		if ( popts->mark_unknown )
			strcat( op, "*" );
		return;
	} else if ( !_strcmpi( op, "INI" ) ) { // INI
		strcpy( op, "INI" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "IND" ) ) { // IND
		strcpy( op, "IND" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "INIR" ) ) { // INIR
		strcpy( op, "INIR" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "INDR" ) ) { // INDR
		strcpy( op, "INDR" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "OUTI" ) ) { // OUTI
		strcpy( op, "OUTI" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "OUTD" ) ) { // OUTD
		strcpy( op, "OUTD" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "OTIR" ) ) { // OTIR
		strcpy( op, "OUTIR" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "OTDR" ) ) { // OTDR
		strcpy( op, "OUTDR" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "LDI" ) ) { // LDI
		strcpy( op, "LDI" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "LDD" ) ) { // LDD
		strcpy( op, "LDD" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "LDIR" ) ) { // LDIR
		strcpy( op, "LDIR" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "LDDR" ) ) { // LDDR
		strcpy( op, "LDDR" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "CPI" ) ) { // CPI
		strcpy( op, "CCI" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "CPD" ) ) { // CPD
		strcpy( op, "CCD" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "CPIR" ) ) { // CPIR
		strcpy( op, "CCIR" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "CPDR" ) ) { // CPDR
		strcpy( op, "CCDR" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "EXX" ) ) { // EXX
		strcpy( op, "EXX" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "NEG" ) ) { // NEG
		strcpy( op, "NEG" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "IM" ) && is_immediate( arg1 ) ) { // IM 0..2
		strcpy( op, "IM0" );
		op[2] += ( char )( parse_int( arg1 ) ); // IM0, IM1, or IM2
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "RETI" ) ) { // RETI
		strcpy( op, "RETI" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "RETN" ) ) { // RETN
		strcpy( op, "RETN" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "RL" ) ) { // RL r - RL (HL) - RL (IX|IY+offset)
		if ( is_reg8( arg1 ) ) {		  // RL r
			strcpy( op, "RALR" );
			arg1 = conv8080reg8( arg1 );
			arg2 = "";
		} else if ( is_Z80_indexed( arg1 ) ) { // RL (IX|IY+offset)
			strcpy( op, "RALX" );
			op[3] = (char)toupper( arg1[2] ); // RLX with X being IX or IY
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized RL format, skip conversion.
		}
	} else if ( !_strcmpi( op, "RLC" ) ) { // RLC r - RLC (HL) - RLC (IX|IY+offset)
		if ( is_reg8( arg1 ) ) {		   // RLC r
			strcpy( op, "RLCR" );
			arg1 = conv8080reg8( arg1 );
			arg2 = "";
		} else if ( is_Z80_indexed( arg1 ) ) { // RLC (IX|IY+offset)
			strcpy( op, "RLCX" );
			op[3] = (char)toupper( arg1[2] ); // RLCX with X being IX or IY
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized RL format, skip conversion.
		}
	} else if ( !_strcmpi( op, "RR" ) ) { // RR r - RR (HL) - RR (IX|IY+offset)
		if ( is_reg8( arg1 ) ) {		  // RR r
			strcpy( op, "RARR" );
			arg1 = conv8080reg8( arg1 );
			arg2 = "";
		} else if ( is_Z80_indexed( arg1 ) ) { // RR (IX|IY+offset)
			strcpy( op, "RARX" );
			op[3] = (char)toupper( arg1[2] ); // RARX with X being IX or IY
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized RL format, skip conversion.
		}
	} else if ( !_strcmpi( op, "RRC" ) ) { // RRC r - RRC (HL) - RRC (IX|IY+offset)
		if ( is_reg8( arg1 ) ) {		   // RRC r
			strcpy( op, "RRCR" );
			arg1 = conv8080reg8( arg1 );
			arg2 = "";
		} else if ( is_Z80_indexed( arg1 ) ) { // RRC (IX|IY+offset)
			strcpy( op, "RRCX" );
			op[3] = (char)toupper( arg1[2] ); // RRCX with X being IX or IY
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized RL format, skip conversion.
		}
	} else if ( !_strcmpi( op, "RLD" ) ) { // RLD
		strcpy( op, "RLD" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "RRD" ) ) { // RRD
		strcpy( op, "RRD" );
		arg1 = arg2 = "";
	} else if ( !_strcmpi( op, "SLA" ) ) { // SLA r - SLA (HL) - SLA (IX|IY+offset)
		if ( is_reg8( arg1 ) ) {		   // SLA r
			strcpy( op, "SLAR" );
			arg1 = conv8080reg8( arg1 );
			arg2 = "";
		} else if ( is_Z80_indexed( arg1 ) ) { // SLA (IX|IY+offset)
			strcpy( op, "SLAX" );
			op[3] = (char)toupper( arg1[2] ); // SLAX with X being IX or IY
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized RL format, skip conversion.
		}
	} else if ( !_strcmpi( op, "SRA" ) ) { // SRA r - SRA (HL) - SRA (IX|IY+offset)
		if ( is_reg8( arg1 ) ) {		   // SRA r
			strcpy( op, "SRAR" );
			arg1 = conv8080reg8( arg1 );
			arg2 = "";
		} else if ( is_Z80_indexed( arg1 ) ) { // SRA (IX|IY+offset)
			strcpy( op, "SRAX" );
			op[3] = (char)toupper( arg1[2] ); // SRAX with X being IX or IY
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized RL format, skip conversion.
		}
	} else if ( !_strcmpi( op, "SRL" ) ) { // SRL r - SRL (HL) - SRL (IX|IY+offset)
		if ( is_reg8( arg1 ) ) {		   // SRL r
			strcpy( op, "SRLR" );
			arg1 = conv8080reg8( arg1 );
			arg2 = "";
		} else if ( is_Z80_indexed( arg1 ) ) { // SRL (IX|IY+offset)
			strcpy( op, "SRLX" );
			op[3] = (char)toupper( arg1[2] ); // SRLX with X being IX or IY
			arg1 = conv8080at( arg1 );
			arg2 = "";
		} else {
			strcat( op, "?" );
			return; // Unrecognized RL format, skip conversion.
		}
	} else if ( !_strcmpi( op, "BIT" ) ) { // BIT
		strcpy( op, "BIT" );
		if ( is_reg8( arg2 ) ) { // BIT b,r
			arg2 = conv8080reg8( arg2 );
		} else if ( is_Z80_indexed( arg2 ) ) { // BIT b,(IX|IY+offset)
			strcpy( op, "BITX" );
			op[3] = (char)toupper( arg2[2] ); // BITX with X being IX or IY
			arg2 = conv8080at( arg2 );
		} else {
			strcat( op, "?" );
			return; // Unrecognized BIT format, skip conversion.
		}
	} else if ( !_strcmpi( op, "RES" ) ) { // RES
		strcpy( op, "RES" );
		if ( is_reg8( arg2 ) ) { // RES b,r
			arg2 = conv8080reg8( arg2 );
		} else if ( is_Z80_indexed( arg2 ) ) { // RES b,(IX|IY+offset)
			strcpy( op, "RESX" );
			op[3] = (char)toupper( arg2[2] ); // RESX with X being IX or IY
			arg2 = conv8080at( arg2 );
		} else {
			strcat( op, "?" );
			return; // Unrecognized RES format, skip conversion.
		}
	} else if ( !_strcmpi( op, "SET" ) ) { // SET
		strcpy( op, "SETB" );
		if ( is_reg8( arg2 ) ) { // SET b,r
			arg2 = conv8080reg8( arg2 );
		} else if ( is_Z80_indexed( arg2 ) ) { // SET b,(IX|IY+offset)
			strcpy( op, "SETX" );
			op[3] = (char)toupper( arg2[2] ); // SETX with X being IX or IY
			arg2 = conv8080at( arg2 );
		} else {
			strcat( op, "?" );
			return; // Unrecognized SET format, skip conversion.
		}
	} else if ( !_strcmpi( op, "JR" ) ) { // JR	[c],nnnn
		if ( is_cond( arg1 ) && arg2[0] ) {
			strcat( op, arg1 );
			arg1 = arg2;
			arg2 = "";
		} else {
			// as is
		}
	} else if ( !_strcmpi( op, "DJNZ" ) ) { // DJNZ	nnnn
		// as is
	} else {
		if ( popts->mark_unknown )
			strcat( op, "#" );
		return;
	}

	// After determining the new opcode and arguments, we update the tokens with the converted values.
	if ( *arg1 ) {
		// if ( count < 2 ) {
		// 	tokens_next( pTokens );
		// 	arg = tokens_getcstr( pTokens, 2 );
		// }
		strcpy( arg, arg1 );
		if ( *arg2 ) {
			strcat( arg, "," );
			strcat( arg, arg2 );
		}
	} else {
		*arg = 0; // Clear the argument if there are no valid arguments after conversion.
	}
}

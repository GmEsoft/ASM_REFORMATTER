#define _CRT_SECURE_NO_WARNINGS

#include "convz80.h"

#include <string.h>
#include <ctype.h>

//=============================================================================
//		convert 8080 to Z-80
//=============================================================================

// The functions below convert 8080 register names and instructions to their Z-80 equivalents.

// Converts 8080 8-bit register names to Z-80 equivalents, handling the special case of "M".
static char *convz80reg8( char *reg )
{
	if ( !_strcmpi( reg, "M" ) )
		return "(HL)";
	return reg;
}

// Converts 8080 16-bit register names to Z-80 equivalents, handling the special case of "PSW".
static char *convz80reg16( char *reg )
{
	if ( !_strcmpi( reg, "B" ) )
		return "BC";
	if ( !_strcmpi( reg, "D" ) )
		return "DE";
	if ( !_strcmpi( reg, "H" ) )
		return "HL";
	if ( !_strcmpi( reg, "PSW" ) )
		return "AF";
	return reg;
}

// Converts an argument to a Z-80 memory reference by wrapping it in parentheses.
static char *convz80at( char *arg )
{
	static char ret[256];
	strcpy( ret, "(" );
	strcat( ret, arg );
	strcat( ret, ")" );
	return ret;
}

// Converts an argument of the form "opX|opY DD" to a Z-80 indexed memory reference "(IX|IY+DD)"
static char *convz80atindex( char *op, char *arg )
{
	static char ret[256];
	strcpy( ret, "(IX" );
	ret[2] = (char)toupper( op[strlen( op ) - 1] ); // IX or IY
	if ( arg[0] != '-' ) {
		strcat( ret, "+" );
	}
	strcat( ret, arg );
	strcat( ret, ")" );
	return ret;
}

// Converts 8080 instructions to their Z-80 equivalents, modifying the tokens in place.
void convz80( tokens_t *pTokens, options_t *popts )
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
	if ( !_strcmpi( op, "JMP" ) ) { // JP	nnnn
		strcpy( op, "JP" );
	} else if ( !_strcmpi( op, "JZ" ) ) { // JP	Z,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "Z";
	} else if ( !_strcmpi( op, "JNZ" ) ) { // JP	NZ,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "NZ";
	} else if ( !_strcmpi( op, "JC" ) ) { // JP	C,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "C";
	} else if ( !_strcmpi( op, "JNC" ) ) { // JP	NC,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "NC";
	} else if ( !_strcmpi( op, "JPO" ) ) { // JP	PO,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "PO";
	} else if ( !_strcmpi( op, "JPE" ) ) { // JP	PE,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "PE";
	} else if ( !_strcmpi( op, "JP" ) ) { // JP	P,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "P";
	} else if ( !_strcmpi( op, "JM" ) ) { // JP	M,nnnn
		strcpy( op, "JP" );
		arg2 = arg1;
		arg1 = "M";
	} else if ( !_strcmpi( op, "PCHL" ) ) { // JP	(HL)
		strcpy( op, "JP" );
		arg1 = "(HL)";
	} else if ( !_strcmpi( op, "CALL" ) ) { // CALL	nnnn
	} else if ( !_strcmpi( op, "CZ" ) ) {	// CALL	Z,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "Z";
	} else if ( !_strcmpi( op, "CNZ" ) ) { // CALL	NZ,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "NZ";
	} else if ( !_strcmpi( op, "CC" ) ) { // CALL	C,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "C";
	} else if ( !_strcmpi( op, "CNC" ) ) { // CALL	NC,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "NC";
	} else if ( !_strcmpi( op, "CPO" ) ) { // CALL	PO,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "PO";
	} else if ( !_strcmpi( op, "CPE" ) ) { // CALL	PE,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "PE";
	} else if ( !_strcmpi( op, "CP" ) ) { // CALL	P,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "P";
	} else if ( !_strcmpi( op, "CM" ) ) { // CALL	M,nnnn
		strcpy( op, "CALL" );
		arg2 = arg1;
		arg1 = "M";
	} else if ( !_strcmpi( op, "RET" ) ) { // RET
	} else if ( !_strcmpi( op, "RZ" ) ) {  // RET	Z
		strcpy( op, "RET" );
		arg1 = "Z";
	} else if ( !_strcmpi( op, "RNZ" ) ) { // RET	NZ
		strcpy( op, "RET" );
		arg1 = "NZ";
	} else if ( !_strcmpi( op, "RC" ) ) { // RET	C
		strcpy( op, "RET" );
		arg1 = "C";
	} else if ( !_strcmpi( op, "RNC" ) ) { // RET	NC
		strcpy( op, "RET" );
		arg1 = "NC";
	} else if ( !_strcmpi( op, "RPO" ) ) { // RET	PO
		strcpy( op, "RET" );
		arg1 = "PO";
	} else if ( !_strcmpi( op, "RPE" ) ) { // RET	PE
		strcpy( op, "RET" );
		arg1 = "PE";
	} else if ( !_strcmpi( op, "RP" ) ) { // RET	P
		strcpy( op, "RET" );
		arg1 = "P";
	} else if ( !_strcmpi( op, "RM" ) ) { // RET	M
		strcpy( op, "RET" );
		arg1 = "M";
	} else if ( !_strcmpi( op, "RST" ) ) { // RST	nn
		switch ( *arg1 ) {
		case '0':
			arg1 = "00H";
			break;
		case '1':
			arg1 = "08H";
			break;
		case '2':
			arg1 = "10H";
			break;
		case '3':
			arg1 = "18H";
			break;
		case '4':
			arg1 = "20H";
			break;
		case '5':
			arg1 = "28H";
			break;
		case '6':
			arg1 = "30H";
			break;
		case '7':
			arg1 = "38H";
			break;
		}
	} else if ( !_strcmpi( op, "MOV" ) ) { // LD	r,r
		strcpy( op, "LD" );
		arg1 = convz80reg8( arg1 );
		arg2 = convz80reg8( arg2 );
	} else if ( !_strcmpi( op, "MVI" ) ) { // LD	r,nn
		strcpy( op, "LD" );
		arg1 = convz80reg8( arg1 );
	} else if ( !_strcmpi( op, "LXI" ) ) { // LD	rr,nnnn
		strcpy( op, "LD" );
		arg1 = convz80reg16( arg1 );
	} else if ( !_strcmpi( op, "LDA" ) ) { // LD	A,(nnnn)
		strcpy( op, "LD" );
		arg2 = convz80at( arg1 );
		arg1 = "A";
	} else if ( !_strcmpi( op, "STA" ) ) { // LD	(nnnn),A
		strcpy( op, "LD" );
		arg1 = convz80at( arg1 );
		arg2 = "A";
	} else if ( !_strcmpi( op, "LDAX" ) ) { // LD	A,(rr)
		strcpy( op, "LD" );
		arg2 = convz80at( convz80reg16( arg1 ) );
		arg1 = "A";
	} else if ( !_strcmpi( op, "STAX" ) ) { // LD	(rr),A
		strcpy( op, "LD" );
		arg1 = convz80at( convz80reg16( arg1 ) );
		arg2 = "A";
	} else if ( !_strcmpi( op, "LHLD" ) ) { // LD	HL,(nnnn)
		strcpy( op, "LD" );
		arg2 = convz80at( arg1 );
		arg1 = "HL";
	} else if ( !_strcmpi( op, "SHLD" ) ) { // LD	(nnnn),HL
		strcpy( op, "LD" );
		arg1 = convz80at( arg1 );
		arg2 = "HL";
	} else if ( !_strcmpi( op, "SPHL" ) ) { // LD	SP,HL
		strcpy( op, "LD" );
		arg1 = "SP";
		arg2 = "HL";
	} else if ( !_strcmpi( op, "XCHG" ) ) { // EX	DE,HL
		strcpy( op, "EX" );
		arg1 = "DE";
		arg2 = "HL";
	} else if ( !_strcmpi( op, "XTHL" ) ) { // EX	(SP),HL
		strcpy( op, "EX" );
		arg1 = "(SP)";
		arg2 = "HL";
	} else if ( !_strcmpi( op, "ADI" )		  // ADD	A,nn
				|| !_strcmpi( op, "ADD" ) ) { // ADD	A,r
		strcpy( op, "ADD" );
		arg2 = convz80reg8( arg1 );
		arg1 = "A";
	} else if ( !_strcmpi( op, "ACI" )		  // ADC	A,nn
				|| !_strcmpi( op, "ADC" ) ) { // ADC	A,r
		strcpy( op, "ADC" );
		arg2 = convz80reg8( arg1 );
		arg1 = "A";
	} else if ( !_strcmpi( op, "SUI" )		  // SUB	nn
				|| !_strcmpi( op, "SUB" ) ) { // SUB	r
		strcpy( op, "SUB" );
		arg1 = convz80reg8( arg1 );
	} else if ( !_strcmpi( op, "SBI" )		  // SBC	A,nn
				|| !_strcmpi( op, "SBB" ) ) { // SBC	A,r
		strcpy( op, "SBC" );
		arg2 = convz80reg8( arg1 );
		arg1 = "A";
	} else if ( !_strcmpi( op, "DAD" ) ) { // ADD	HL,rr
		strcpy( op, "ADD" );
		arg2 = convz80reg16( arg1 );
		arg1 = "HL";
	} else if ( !_strcmpi( op, "DI" )		  // DI
				|| !_strcmpi( op, "EI" )	  // EI
				|| !_strcmpi( op, "NOP" ) ) { // NOP
	} else if ( !_strcmpi( op, "HLT" ) ) {	  // HALT
		strcpy( op, "HALT" );
	} else if ( !_strcmpi( op, "INR" ) ) { // INC	r
		strcpy( op, "INC" );
		arg1 = convz80reg8( arg1 );
	} else if ( !_strcmpi( op, "DCR" ) ) { // DEC	r
		strcpy( op, "DEC" );
		arg1 = convz80reg8( arg1 );
	} else if ( !_strcmpi( op, "INX" ) ) { // INC	rr
		strcpy( op, "INC" );
		arg1 = convz80reg16( arg1 );
	} else if ( !_strcmpi( op, "DCX" ) ) { // DEC	rr
		strcpy( op, "DEC" );
		arg1 = convz80reg16( arg1 );
	} else if ( !_strcmpi( op, "DAA" ) ) { // DAA
	} else if ( !_strcmpi( op, "CMA" ) ) { // CPL
		strcpy( op, "CPL" );
	} else if ( !_strcmpi( op, "STC" ) ) { // SCF
		strcpy( op, "SCF" );
	} else if ( !_strcmpi( op, "CMC" ) ) { // CCF
		strcpy( op, "CCF" );
	} else if ( !_strcmpi( op, "RLC" ) ) { // RLCA
		strcpy( op, "RLCA" );
	} else if ( !_strcmpi( op, "RRC" ) ) { // RRCA
		strcpy( op, "RRCA" );
	} else if ( !_strcmpi( op, "RAL" ) ) { // RLA
		strcpy( op, "RLA" );
	} else if ( !_strcmpi( op, "RAR" ) ) { // RRA
		strcpy( op, "RRA" );
	} else if ( !_strcmpi( op, "ANI" )		  // AND	nn
				|| !_strcmpi( op, "ANA" ) ) { // AND	r
		strcpy( op, "AND" );
		arg1 = convz80reg8( arg1 );
	} else if ( !_strcmpi( op, "XRI" )		  // XOR	nn
				|| !_strcmpi( op, "XRA" ) ) { // XOR	r
		strcpy( op, "XOR" );
		arg1 = convz80reg8( arg1 );
	} else if ( !_strcmpi( op, "ORI" )		  // OR	nn
				|| !_strcmpi( op, "ORA" ) ) { // OR	r
		strcpy( op, "OR" );
		arg1 = convz80reg8( arg1 );
	} else if ( !_strcmpi( op, "CPI" )		  // CP	nn
				|| !_strcmpi( op, "CMP" ) ) { // CP	r
		strcpy( op, "CP" );
		arg1 = convz80reg8( arg1 );
	} else if ( !_strcmpi( op, "PUSH" )		  // PUSH	rr
				|| !_strcmpi( op, "POP" ) ) { // POP	rr
		arg1 = convz80reg16( arg1 );
	} else if ( !_strcmpi( op, "IN" ) ) { // IN	A,(nn)
		arg2 = convz80at( arg1 );
		arg1 = "A";
	} else if ( !_strcmpi( op, "OUT" ) ) { // OUT	(nn),A
		arg1 = convz80at( arg1 );
		arg2 = "A";
	} else if ( !popts->conv_z80lib ) {
		// If the instruction is not recognized and we're not using z80lib conventions, we skip conversion.
		if ( popts->mark_unknown )
			strcat( op, "*" );
		return;
		// If we're using z80lib conventions, we check for known macros and convert them.	}
	} else if ( !_strcmpi( op, "LDX" ) ||
				!_strcmpi( op, "LDY" ) ) { // LD r,(IX|IY+dd)
		arg1 = convz80reg8( arg1 );
		arg2 = convz80atindex( op, arg2 );
		strcpy( op, "LD" );
	} else if ( !_strcmpi( op, "STX" ) ||
				!_strcmpi( op, "STY" ) ) { // LD (IX|IY+dd),r
		char *t = convz80atindex( op, arg2 );
		arg2 = convz80reg8( arg1 );
		arg1 = t;
		strcpy( op, "LD" );
	} else if ( !_strcmpi( op, "MVIX" ) ||
				!_strcmpi( op, "MVIY" ) ) { // LD (IX|IY+dd),nn
		char *t = convz80atindex( op, arg2 );
		arg2 = arg1;
		arg1 = t;
		strcpy( op, "LD" );
	} else if ( !_strcmpi( op, "LXIX" ) ||
				!_strcmpi( op, "LXIY" ) ) { // LD	IX|IY,nnnn
		arg2 = arg1;
		arg1 = "??";
		arg1[0] = (char)toupper( op[2] ); // IX, IY
		arg1[1] = (char)toupper( op[3] );
		strcpy( op, "LD" );
	} else if ( !_strcmpi( op, "SPIX" ) ||
				!_strcmpi( op, "SPIY" ) ) { // LD	SP,IX|IY
		arg1 = "SP";
		arg2 = "??";
		arg2[0] = (char)toupper( op[2] ); // IX, IY
		arg2[1] = (char)toupper( op[3] );
		strcpy( op, "LD" );
	} else if ( !_strcmpi( op, "LBCD" ) ||
				!_strcmpi( op, "LDED" ) ||
				!_strcmpi( op, "LIXD" ) ||
				!_strcmpi( op, "LIYD" ) ||
				!_strcmpi( op, "LSPD" ) ) { // LD	BC|DE|IX|IY|SP,(nnnn)
		arg2 = convz80at( arg1 );
		arg1 = "??";
		arg1[0] = (char)toupper( op[1] ); // BC, DD, IX, IY, or SP
		arg1[1] = (char)toupper( op[2] );
		strcpy( op, "LD" );
	} else if ( !_strcmpi( op, "SBCD" ) ||
				!_strcmpi( op, "SDED" ) ||
				!_strcmpi( op, "SIXD" ) ||
				!_strcmpi( op, "SIYD" ) ||
				!_strcmpi( op, "SSPD" ) ) { // LD	(nnnn),BC|DE|IX|IY|SP
		arg1 = convz80at( arg1 );
		arg2 = "??";
		arg2[0] = (char)toupper( op[1] ); // BC, DD, IX, IY, or SP
		arg2[1] = (char)toupper( op[2] );
		strcpy( op, "LD" );
	} else if ( !_strcmpi( op, "LDAI" ) ||
				!_strcmpi( op, "LDAR" ) ) { // LD A,I or LD A,R
		arg1 = "A";
		arg2 = "?";
		arg2[0] = (char)toupper( op[3] ); // I or R
		strcpy( op, "LD" );
	} else if ( !_strcmpi( op, "STAI" ) ||
				!_strcmpi( op, "STAR" ) ) { // LD I,A or LD R,A
		arg2 = "A";
		arg1 = "?";
		arg1[0] = (char)toupper( op[3] ); // I or R
		strcpy( op, "LD" );
	} else if ( !_strcmpi( op, "ADCX" ) ||
				!_strcmpi( op, "ADCY" ) ) { // ADC A,(IX|IY) or ADC A,(IX|IY)
		arg2 = convz80atindex( op, arg1 );
		strcpy( op, "ADC" );
		arg1 = "A";
	} else if ( !_strcmpi( op, "ADDX" ) ||
				!_strcmpi( op, "ADDY" ) ) { // ADD A,(IX|IY) or ADD A,(IX|IY)
		arg2 = convz80atindex( op, arg1 );
		arg1 = "A";
		strcpy( op, "ADD" );
	} else if ( !_strcmpi( op, "SBCX" ) ||
				!_strcmpi( op, "SBCY" ) ) { // SBC A,(IX|IY) or SBC A,(IX|IY)
		arg2 = convz80atindex( op, arg1 );
		arg1 = "A";
		strcpy( op, "SBC" );
	} else if ( !_strcmpi( op, "SUBX" ) ||
				!_strcmpi( op, "SUBY" ) ) { // SUB A,(IX|IY) or SUB A,(IX|IY)
		arg1 = convz80atindex( op, arg1 );
		strcpy( op, "SUB" );
	} else if ( !_strcmpi( op, "CMPX" ) ||
				!_strcmpi( op, "CMPY" ) ) { // CP A,(IX|IY) or CP A,(IX|IY)
		arg1 = convz80atindex( op, arg1 );
		strcpy( op, "CP" );
	} else if ( !_strcmpi( op, "ANDX" ) ||
				!_strcmpi( op, "ANDY" ) ) { // AND A,(IX|IY) or AND A,(IX|IY)
		arg1 = convz80atindex( op, arg1 );
		strcpy( op, "AND" );
	} else if ( !_strcmpi( op, "ORX" ) ||
				!_strcmpi( op, "ORY" ) ) { // OR A,(IX|IY) or OR A,(IX|IY)
		arg1 = convz80atindex( op, arg1 );
		strcpy( op, "OR" );
	} else if ( !_strcmpi( op, "XORX" ) ||
				!_strcmpi( op, "XORY" ) ) { // XOR A,(IX|IY) or XOR A,(IX|IY)
		arg1 = convz80atindex( op, arg1 );
		strcpy( op, "XOR" );
	} else if ( !_strcmpi( op, "DADC" ) ) { // DADC HL,reg16
		arg2 = convz80reg16( arg1 );
		arg1 = "HL";
		strcpy( op, "ADC" );
	} else if ( !_strcmpi( op, "DSBC" ) ) { // DSBC HL,reg16
		arg2 = convz80reg16( arg1 );
		arg1 = "HL";
		strcpy( op, "SBC" );
	} else if ( !_strcmpi( op, "DADX" ) ||
				!_strcmpi( op, "DADY" ) ) { // ADD IX|IY,reg16
		arg2 = convz80reg16( arg1 );
		arg1 = "IX";									 // or "IY", depending on the specific instruction
		arg1[1] = (char)toupper( op[strlen( op ) - 1] ); // IX or IY
		strcpy( op, "ADD" );
	} else if ( !_strcmpi( op, "BIT" ) ||
				!_strcmpi( op, "RES" ) ||
				!_strcmpi( op, "SETB" ) ) { // BIT n,r - RES n,r - SET n,r
		arg2 = convz80reg8( arg2 );
		op[3] = '\0'; // Remove the B suffix from the opcode to get the base instruction (BIT, RES, or SET)
	} else if ( !_strcmpi( op, "BITX" ) ||
				!_strcmpi( op, "BITY" ) ||
				!_strcmpi( op, "RESX" ) ||
				!_strcmpi( op, "RESY" ) ||
				!_strcmpi( op, "SETX" ) ||
				!_strcmpi( op, "SETY" ) ) { // BIT n,(IX|IY)
		arg2 = convz80atindex( op, arg2 );
		op[3] = '\0'; // Remove the X or Y suffix from the opcode to get the base instruction (BIT, RES, or SET)
	} else if ( !_strcmpi( op, "DCRX" ) ||
				!_strcmpi( op, "DCRY" ) ) { // DEC (IX|IY+dd)
		arg1 = convz80atindex( op, arg1 );
		strcpy( op, "DEC" );
	} else if ( !_strcmpi( op, "DCXIX" ) ||
				!_strcmpi( op, "DCXIY" ) ) { // DEC IX|IY
		arg1 = "IX";
		arg1[1] = (char)toupper( op[strlen( op ) - 1] );
		strcpy( op, "DEC" );
	} else if ( !_strcmpi( op, "INRX" ) ||
				!_strcmpi( op, "INRY" ) ) { // INC (IX|IY+dd)
		arg1 = convz80atindex( op, arg1 );
		strcpy( op, "INC" );
	} else if ( !_strcmpi( op, "INXIX" ) ||
				!_strcmpi( op, "INXIY" ) ) { // INC IX|IY
		arg1 = "IX";
		arg1[1] = (char)toupper( op[strlen( op ) - 1] );
		strcpy( op, "INC" );
	} else if ( !_strcmpi( op, "POPIX" ) ||
				!_strcmpi( op, "POPIY" ) ) { // POP IX|IY
		arg1 = "IX";
		arg1[1] = (char)toupper( op[strlen( op ) - 1] );
		strcpy( op, "POP" );
	} else if ( !_strcmpi( op, "PUSHIX" ) ||
				!_strcmpi( op, "PUSHIY" ) ) { // PUSH IX|IY
		arg1 = "IX";
		arg1[1] = (char)toupper( op[strlen( op ) - 1] );
		strcpy( op, "PUSH" );
	} else if ( !_strcmpi( op, "PCIX" ) ||
				!_strcmpi( op, "PCIY" ) ) { // JP (IX|IY)
		arg1 = "(IX)";
		arg1[2] = (char)toupper( op[strlen( op ) - 1] );
		strcpy( op, "JP" );
	} else if ( !_strcmpi( op, "DJNZ" ) ||
				!_strcmpi( op, "JR" ) ) { // DJNZ dest - JR dest
										  // as is
	} else if ( !_strcmpi( op, "JRZ" ) ||
				!_strcmpi( op, "JRNZ" ) ||
				!_strcmpi( op, "JRC" ) ||
				!_strcmpi( op, "JRNC" ) ) { // JR cc,dest
		arg2 = arg1;
		arg1 = "??";					  // Placeholder for the condition code, as JR does not support all the same conditions as JP.
		arg1[0] = (char)toupper( op[2] ); // Z, NZ, C, NC
		arg1[1] = (char)toupper( op[3] ); // Z, NZ, C, NC;
		strcpy( op, "JR" );
	} else if ( !_strcmpi( op, "DJNZ" ) ) { // DJNZ dest
		strcpy( op, "DJNZ" );
	} else if ( !_strcmpi( op, "NEG" ) ) { // NEG
		strcpy( op, "NEG" );
	} else if ( !_strcmpi( op, "RETI" ) ) { // RETI
		strcpy( op, "RETI" );
	} else if ( !_strcmpi( op, "RETN" ) ) { // RETN
		strcpy( op, "RETN" );
	} else if ( !_strcmpi( op, "RALR" ) ||
				!_strcmpi( op, "RARR" ) ) { // RL|RR r
		arg1 = convz80reg8( arg1 );
		op[1] = op[2]; // RL or RR
		op[2] = '\0';
	} else if ( !_strcmpi( op, "RALX" ) ||
				!_strcmpi( op, "RALY" ) ||
				!_strcmpi( op, "RARX" ) ||
				!_strcmpi( op, "RARY" ) ) { // RL|RR (IX|IY+dd)
		arg1 = convz80atindex( op, arg1 );
		op[1] = op[2]; // RL or RR
		op[2] = '\0';
	} else if ( !_strcmpi( op, "RLCR" ) ||
				!_strcmpi( op, "RRCR" ) ||
				!_strcmpi( op, "SLAR" ) ||
				!_strcmpi( op, "SRAR" ) ||
				!_strcmpi( op, "SRLR" ) ) { // RLC|RRC|SLA|SRA|SRL r
		arg1 = convz80reg8( arg1 );
		op[3] = '\0';
	} else if ( !_strcmpi( op, "RLCX" ) ||
				!_strcmpi( op, "RLCY" ) ||
				!_strcmpi( op, "RRCX" ) ||
				!_strcmpi( op, "RRCY" ) ||
				!_strcmpi( op, "SLAX" ) ||
				!_strcmpi( op, "SLAY" ) ||
				!_strcmpi( op, "SRAX" ) ||
				!_strcmpi( op, "SRAY" ) ||
				!_strcmpi( op, "SRLX" ) ||
				!_strcmpi( op, "SRLY" ) ) { // RLC|RRC|SLA|SRA|SRL (IX|IY+dd)
		arg1 = convz80atindex( op, arg1 );
		op[3] = '\0';
	} else if ( !_strcmpi( op, "RLD" ) ) { // RLD
		strcpy( op, "RLD" );
	} else if ( !_strcmpi( op, "RRD" ) ) { // RRD
		strcpy( op, "RRD" );
	} else if ( !_strcmpi( op, "XTIX" ) ||
				!_strcmpi( op, "XTIY" ) ) { // EX (SP),IX or EX (SP),IY
		arg1 = "(SP)";
		arg2 = "IX";
		arg2[1] = (char)toupper( op[strlen( op ) - 1] );
		strcpy( op, "EX" );
	} else if ( !_strcmpi( op, "EXAF" ) ) { // EX AF,AF'
		strcpy( op, "EX" );
		arg1 = "AF";
		arg2 = "AF'";
	} else if ( !_strcmpi( op, "EXX" ) ) { // EXX
		strcpy( op, "EXX" );
	} else if ( !_strcmpi( op, "IM0" ) ||
				!_strcmpi( op, "IM1" ) ||
				!_strcmpi( op, "IM2" ) ) { // IM 0, IM 1, IM 2
		arg1 = "?";						   // Placeholder for the interrupt mode, as the original instruction specifies it in the opcode.
		arg1[0] = op[2];				   // 0, 1, or 2
		strcpy( op, "IM" );
	} else if ( !_strcmpi( op, "INP" ) ) { // IN r,(C)
		strcpy( op, "IN" );
		arg1 = convz80reg8( arg1 );
		arg2 = "(C)";
	} else if ( !_strcmpi( op, "OUTP" ) ) { // OUT (C),r
		strcpy( op, "OUT" );
		arg2 = convz80reg8( arg1 );
		arg1 = "(C)";
	} else if ( !_strcmpi( op, "CCD" ) ) { // CPD
		strcpy( op, "CPD" );
	} else if ( !_strcmpi( op, "CCDR" ) ) { // CPDR
		strcpy( op, "CPDR" );
	} else if ( !_strcmpi( op, "CCI" ) ) { // CPI
		strcpy( op, "CPI" );
	} else if ( !_strcmpi( op, "CCIR" ) ) { // CPIR
		strcpy( op, "CPIR" );
	} else if ( !_strcmpi( op, "IND" ) ) { // IND
		strcpy( op, "IND" );
	} else if ( !_strcmpi( op, "INDR" ) ) { // INDR
		strcpy( op, "INDR" );
	} else if ( !_strcmpi( op, "INI" ) ) { // INI
		strcpy( op, "INI" );
	} else if ( !_strcmpi( op, "INIR" ) ) { // INIR
		strcpy( op, "INIR" );
	} else if ( !_strcmpi( op, "LDD" ) ) { // LDD
		strcpy( op, "LDD" );
	} else if ( !_strcmpi( op, "LDDR" ) ) { // LDDR
		strcpy( op, "LDDR" );
	} else if ( !_strcmpi( op, "LDI" ) ) { // LDI
		strcpy( op, "LDI" );
	} else if ( !_strcmpi( op, "LDIR" ) ) { // LDIR
		strcpy( op, "LDIR" );
	} else if ( !_strcmpi( op, "OUTD" ) ) { // OUTD
		strcpy( op, "OUTD" );
	} else if ( !_strcmpi( op, "OUTDR" ) ) { // OUTDR
		strcpy( op, "OTDR" );
	} else if ( !_strcmpi( op, "OUTI" ) ) { // OUTI
		strcpy( op, "OUTI" );
	} else if ( !_strcmpi( op, "OUTIR" ) ) { // OUTIR
		strcpy( op, "OTIR" );
	} else { // TODO: Implement z80lib macro conversions here, if needed.
		// For now, we just skip conversion for unrecognized instructions.
		if ( popts->mark_unknown )
			strcat( op, "*" );
		return;
	}

	// After determining the new opcode and arguments, we update the tokens with the converted values.
	if ( *arg1 ) {
		if ( count < 2 ) {
			tokens_next( pTokens );
			arg = tokens_getcstr( pTokens, 2 );
		}
		strcpy( arg, arg1 );
		if ( *arg2 ) {
			strcat( arg, "," );
			strcat( arg, arg2 );
		}
	} else {
		*arg = 0; // Clear the argument if there are no valid arguments after conversion.
	}
}

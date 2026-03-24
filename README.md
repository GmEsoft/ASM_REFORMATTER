# ASM Reformatter and 8080 to Z-80 Code Conversion Tool

A command-line tool to reformat assembly language source code with configurable formatting options. Supports 8080 to Z-80 assembly code conversion.

## Features

- **Configurable alignment**: Adjustable tab positions for labels, mnemonics, and comments
- **Case conversion**: Convert mnemonics and register names to upper or lower case (preserves literal and comment case)
- **Multi-statement support**: Handle multiple statements per line with configurable separator character
- **8080 to Z-80 conversion**: Automatically convert Intel 8080 assembly code to Z-80 equivalents
  - Converts 8080 mnemonics (JMP → JP, MOV → LD, etc.)
  - Converts 8080 registers (M → (HL), PSW → AF, etc.)
  - Converts instruction syntax to Z-80 format
- **Comment alignment**: Option to align no-instruction comments with the mnemonics column
- **Preserves code semantics**: Maintains code functionality while improving readability

## Usage

```ps
reformat -I:infile -O:outfile [opts...]
```

### Required Arguments

- `-I:infile` - Input assembly source file
- `-O:outfile` - Output reformatted assembly file

### Options

- `-T:n` - Tab size in spaces (default: 8)
- `-M:n` - Mnemonics column tab position (default: 2)
- `-C:n` - Comments column tab position (default: 4)
- `-U` - Convert to upper-case (mnemonics and registers only)
- `-L` - Convert to lower-case (mnemonics and registers only)
- `-S:c` - Multi-statement separator character (default: '!')
- `-XZ` - Convert 8080 assembly code to Z-80 code
- `-XS` - Allow separator character in comments
- `-XN` - Align no-instruction comments with mnemonics column
- `-?` - Display help message

## Examples

### Basic reformatting with upper-case and custom tab size:
```ps
reformat -U -T:4 -I:input.asm -O:output.asm
```

### Convert 8080 code to Z-80 with upper-case:
```ps
reformat -U -XZ -T:4 -M:2 -C:4 -I:old_8080.asm -O:new_z80.asm
```

### Reformat with lower-case mnemonics:
```ps
reformat -L -T:8 -M:2 -I:input.asm -O:output.asm
```

## Input/Output Example

**Input:**
```asm
    .org    CBASE - 32      ; iLoad starting address
    ld      sp, $80         ; Set local SP
    ; Mount th SD
    ld      a, SDMOUNT_OPC  ; Select SDMOUNT opcode (IOS)
    out     (STO_OPCD), a
    in      a, (EXC_RD_OPCD); Call it
    ld      a, SDMOUNT_OPC  ; Select SDMOUNT opcode (IOS)
    out     (STO_OPCD), a
    in      a, (EXC_RD_OPCD); Call it
    jp      BOOT            ; Jump to BIOS Cold Boot
```

**Output (with `-U -T:8 -M:1 -C:4`):**
```asm
        .ORG    CBASE - 32      ; iLoad starting address
        LD      SP, $80         ; Set local SP
                                ; Mount th SD
        LD      A, SDMOUNT_OPC  ; Select SDMOUNT opcode (IOS)
        OUT     (STO_OPCD), A
        IN      A, (EXC_RD_OPCD); Call it
        LD      A, SDMOUNT_OPC  ; Select SDMOUNT opcode (IOS)
        OUT     (STO_OPCD), A
        IN      A, (EXC_RD_OPCD); Call it
        JP      BOOT            ; Jump to BIOS Cold Boot
```

## 8080 to Z-80 Conversion

When using the `-XZ` option, the tool converts:

- **Jump instructions**: JMP → JP, JZ → JP Z, JC → JP C, etc.
- **Registers**: B → BC, D → DE, H → HL, PSW → AF
- **Memory references**: M → (HL)
- **Arithmetic/Logic**: ADD → ADD A, SUB → SUB, ANA → AND, XRA → XOR, ORA → OR
- ...

**Input:**
```asm
	noread:	;enter here from submit file
	;set the last character to zero for later scans
	lxi h,comlen! mov b,m ;length is in b
	readcom0: inx h! mov a,b! ora a ;end of scan?
		jz readcom1! mov a,m ;get character and translate
		call translate! mov m,a! dcr b! jmp readcom0
		;
	readcom1: ;end of scan, h,l address end of command
		mov m,a ;store a zero
		lxi h,combuf! shld comaddr ;ready to scan to zero
	ret
```

**Output (with `-u -m:1 -c:4 -s:! -xs -xn -xz`):**
```asm
NOREAD:	;enter here from submit file
        ;set the last character to zero for later scans
        LD      HL,COMLEN
        LD      B,(HL)          ;length is in b
READCOM0:INC    HL
        LD      A,B
        OR      A               ;end of scan?
        JP      Z,READCOM1
        LD      A,(HL)          ;get character and translate
        CALL    TRANSLATE
        LD      (HL),A
        DEC     B
        JP      READCOM0
        ;
READCOM1:;end of scan, h,l address end of command
        LD      (HL),A          ;store a zero
        LD      HL,COMBUF
        LD      (COMADDR),HL    ;ready to scan to zero
        RET
```


## License

GNU General Public License v3.0 or later

## Author

GmEsoft (2026)

## Version

0.2

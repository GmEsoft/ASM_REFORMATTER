# ASM Reformatter

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
reformat -I:infile [-O:outfile] [opts...]
```

### Required Arguments

- `-I:infile` - Input assembly source file
- `-O:outfile` - Output reformatted assembly file (default: stdout if not specified)

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
start:jmp	main
main:	mov	a,b	;move b to a
add a,c
ret
```

**Output (with `-U -T:4 -M:2 -C:4`):**
```asm
START:
    JP	MAIN
MAIN:
    MOV	A,B		;move b to a
    ADD	A,C
    RET
```

## 8080 to Z-80 Conversion

When using the `-XZ` option, the tool converts:

- **Jump instructions**: JMP → JP, JZ → JP Z, JC → JP C, etc.
- **Registers**: B → BC, D → DE, H → HL, PSW → AF
- **Memory references**: M → (HL)
- **Arithmetic/Logic**: ADD → ADD A, SUB → SUB, ANA → AND, XRA → XOR, ORA → OR
- ...

## License

GNU General Public License v3.0 or later

## Author

GmEsoft (2026)

## Version

0.2

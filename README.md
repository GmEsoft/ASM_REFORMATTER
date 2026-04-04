# ASM Reformatter and 8080 from/to Z-80 Code Conversion Tool

A command-line tool to reformat assembly language source code with configurable formatting options. Supports 8080 from/to Z-80 assembly code conversion.

## Features

- **Configurable alignment**: Adjustable tab positions for labels, mnemonics, and comments
- **Case conversion**: Convert mnemonics and register names to upper or lower case (preserves literal and comment case)
- **Multi-statement support**: Handle multiple statements per line with configurable separator character
- **8080 to Z-80 conversion**: Automatically convert Intel 8080 assembly code to Z-80 equivalents
  - Converts 8080 mnemonics (JMP → JP, MOV → LD, etc.)
  - Converts 8080 registers (M → (HL), PSW → AF, etc.)
  - Converts instruction syntax to Z-80 format
- **Z-80 to 8080 conversion**: Automatically convert Z-80 assembly code to Intel 8080 equivalents
  - Converts Z-80 mnemonics (JP → JMP, LD → MOV, etc.)
  - Converts Z-80 registers ((HL) → M, AF → PSW, etc.)
  - Converts instruction syntax to Z-80 format
  - Optionally converts Z-80 extended instructions to Z80.LIB macro invocationss
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
- `-M:n` - Mnemonics column tab position (default: 2) - the leftmost culumn is numbered 0
- `-C:n` - Comments column tab position (default: 4) - the leftmost culumn is numbered 0
- `-U`   - Convert to upper-case (mnemonics and registers only)
- `-L`   - Convert to lower-case (mnemonics and registers only)
- `-S:c` - Multi-statement separator character (default: '!')
- `-XE`  - Add EOF char at end of file
- `-XN`  - Align no-instruction comments with mnemonics column
- `-XS`  - Allow separator character in comments
- `-XZ`  - Convert 8080 assembly code to Z-80 code
- `-X8`  - Convert Z-80 assembly code to 8080 code
- `-X*`  - Mark unrecognized mnemonics
- `-?`   - Display help message

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

- **Jump instructions**: `JMP` → `JP`, `JZ` → `JP Z,`, `JC → JP C,`, etc.
- **Registers**: `B` → `BC`, `D` → `DE`, `H` → `HL`, `PSW` → `AF`
- **Memory references**: `M` → `(HL)`
- **Arithmetic/Logic**: `ADD` → `ADD A,`, `SBB` → `SBC A,`, `ANA` → `AND`, `XRA` → `XOR`, `ORA` → `OR`
- ...

When the option `-SL` is added, the following macro calls
are also converted to their equivalent Z-80 instructions:

- **Jump instructions**: `JRZ` → `JR Z,`, `PCIX` → `JP (IX)`, etc.
- **Indexed addressing**: `LDX r,d` → `LD r,(IX+d)`, `STY r,d` → `LD (IY+d),r`, etc.
- **Arithmetic**: `DSBC r` → `SBC HL,rr`, `DADX r` → `ADD IX,rr`, etc.
- **Block instructions**: `CCI` → `CPI`, `OUTIR` → `OTIR`, etc.
- ...

**Input:**
```asm
        noread: ;enter here from submit file
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
NOREAD: ;enter here from submit file
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

## Z-80 to 8080 Conversion

When using the `-X8` option, the tool converts:

- **Jump instructions**: `JP` → `JMP`, `JP Z,` → `JZ`, `JP C,` → `JC`, etc.
- **Registers**: `BC` → `B`, `DE` → `D`, `HL` → `H`, `AF` → `PSW`
- **Memory references**: `(HL)` → `M`
- **Arithmetic/Logic**: `ADD A,` → `ADD`, `SBC A,` → `SBB`, `AND` → `ANA`, `XOR` → `XRA`, `OR` → `ORA`
- ...

If the `-XL` is provided, the extended Z-80 instructions are converted to `Z80.LIB`
macro invocations:

- `LD IX,nnnn` → `LXIX nnnn`
- `JR Z,nnnn` → `JRZ nnnn`
- `EX AF,AF'` → `EXAF`
- `SET n,r` → `SETB n,r`
- ...

Note that the original Z80.LIB macros library from DRI has an error in the calculation
formula for JR displacements:

```asm
JR      MACRO   ?N
        DB      18H,?N-$-1
        ENDM
```

This has been fixed to (the JR instuctions with their displacement value take 2 bytes):

```asm
JR      MACRO   ?N              ;JR     ?N
        DB      18H,?N-$-2
        ENDM
```

**Input:**

Note the conversion of the statements `OTIR` and `IN A,(C)`
to macro calls `OUTIR` and `INP A`. If the statement
`MACLIB Z80` is added at the beginning of the output source
file, it can be assembled using `RMAC`. For M80, add a line
containing `INCLUDE Z80.LIB`.

```asm
STREAM$OUT:
        LD      A,(HL)
        OR      A
        RET     Z
        LD      B,A
        INC     HL
        LD      C,(HL)
        INC     HL
        OTIR
        JP      STREAM$OUT

?CIST:                          ; character input status

        LD      A,B
        CP      6
        JP      NC,NULL$STATUS  ; can't read from centronics
        LD      L,B
        LD      H,0             ; make device number 16 bits
        LD      DE,DATA$PORTS
        ADD     HL,DE           ; make pointer to port address
        LD      C,(HL)
        INC     C               ; get SIO status port
        IN      A,(C)           ; read from status port
        AND     1               ; isolate RxRdy
        RET     Z               ; return with zero
        OR      0FFH
        RET
```

**Output: (with `-u -m:1 -c:4 -s:! -xs -xn -xz -xl`)**
```asm
STREAM$OUT:
        MOV     A,M
        ORA     A
        RZ
        MOV     B,A
        INX     H
        MOV     C,M
        INX     H
        OUTIR
        JMP     STREAM$OUT

?CIST:                          ; character input status

        MOV     A,B
        CPI     6
        JNC     NULL$STATUS     ; can't read from centronics
        MOV     L,B
        MVI     H,0             ; make device number 16 bits
        LXI     D,DATA$PORTS
        DAD     D               ; make pointer to port address
        MOV     C,M
        INR     C               ; get SIO status port
        INP     A               ; read from status port
        ANI     1               ; isolate RxRdy
        RZ                      ; return with zero
        ORI     0FFH
        RET
```


## License

GNU General Public License v3.0 or later

## Author

GmEsoft (2026)

## Version

0.6

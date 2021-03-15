#include <intel8080.h>
#include <stdio.h>
#include <stdlib.h>

#define TOTAL_MEMORY  (64 * 1024)
#define START_ADDRESS (0x0000)

typedef enum
{
    B = 0,
    C,
    D,
    E,
    H,
    L,
    A,
    TOTAL_REGISTERS
}MicroprocRegister;

__attribute__((aligned(8)))
typedef struct
{
    uint8_t carry     : 1;
    uint8_t aux_carry : 1;
    uint8_t sign      : 1;
    uint8_t zero      : 1;
    uint8_t parity    : 1;
}StatusBits;

typedef struct
{
    uint8_t memory[TOTAL_MEMORY];
    uint8_t registers[TOTAL_REGISTERS];
    uint32_t cycles;
    uint16_t pc;
    uint16_t sp;
    StatusBits flags;
    uint8_t init;
}MicroprocState;

static MicroprocState __attribute__((aligned(64))) Intel8080 = { 0 };

static uint8_t InstructionCycles[] =
{
//         0   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
/* 0 */    4, 10,  7,  5,  5,  5,  7,  4,  4, 10,  7,  5,  5,  5,  7,  4,
/* 1 */    4, 10,  7,  5,  5,  5,  7,  4,  4, 10,  7,  5,  5,  5,  7,  4,
/* 2 */    4, 10, 16,  5,  5,  5,  7,  4,  4, 10, 16,  5,  5,  5,  7,  4,
/* 3 */    4, 10, 13,  5, 10, 10, 10,  4,  4, 10, 13,  5,  5,  5,  7,  4,
/* 4 */    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,
/* 5 */    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,
/* 6 */    5,  5,  5,  5,  5,  5,  7,  5,  5,  5,  5,  5,  5,  5,  7,  5,
/* 7 */    7,  7,  7,  7,  7,  7,  7,  7,  5,  5,  5,  5,  5,  5,  7,  5,
/* 8 */    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
/* 9 */    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
/* A */    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
/* B */    4,  4,  4,  4,  4,  4,  7,  4,  4,  4,  4,  4,  4,  4,  7,  4,
/* C */    5, 10, 10, 10, 11, 11,  7, 11,  5, 10, 10, 10, 11, 17,  7, 11,
/* D */    5, 10, 10, 10, 11, 11,  7, 11,  5, 10, 10, 10, 11, 17,  7, 11,
/* E */    5, 10, 10, 18, 11, 11,  7, 11,  5,  5, 10,  5, 11, 17,  7, 11,
/* F */    5, 10, 10,  4, 11, 11,  7, 11,  5,  5, 10,  4, 11, 17,  7, 11,
}

static MicroprocError LoadBinary(const char *binary_name)
{
    MicroprocError ret = SUCCESSFUL;
    if(NULL == binary_name)
    {
        printf("NULL binary_name parameter\n");
        ret = INVALID_BINARY;
    }
    else
    {
        FILE *file = fopen(binary_name, "rb");
        if(NULL == file)
        {
            printf("Error opening file %s\n", binary_name);
            ret = INVALID_BINARY;
        }
        else
        {
            if(0 == fseek(file, 0L, SEEK_END))
            {
                size_t binary_size = ftell(file);
                rewind(file);
                if(binary_size + START_ADDRESS >= TOTAL_MEMORY)
                {
                    printf("Not enough memory available to load the binary\n");
                    ret = MEMORY_FAIL;
                }
                else
                {
                    fread(&Intel8080.memory[START_ADDRESS], sizeof(uint8_t), binary_size, file);
                    printf("Loaded %u bytes\n", binary_size);
                }
            }
            else
            {
                printf("Error seeking to EOF\n");
                ret = IO_ERROR;
            }
            if(0 != fclose(file))
            {
                printf("Error closing file %s\n");
                ret = IO_ERROR;
            }
        }
    }
    return ret;
}

#if defined(DEBUG)
static void DisassembleInstruction(uint8_t *instruction)
{
    if(NULL == instruction)
    {
        printf("NULL instruction parameter\n");
        return;
    }
    switch(*instruction)
    {
        case 0x00: printf("NOP\n"); break;
        case 0x01: printf("LXI B, %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0x02: printf("STAX B\n"); break;
        case 0x03: printf("INX B\n"); break;
        case 0x04: printf("INR B\n"); break;
        case 0x05: printf("DCR B\n"); break;
        case 0x06: printf("MVI B, %#02X\n", instruction[1]); break;
        case 0x07: printf("RLC\n"); break;
        case 0x08: printf("*NOP\n"); break;
        case 0x09: printf("DAD B\n"); break;
        case 0x0A: printf("LDAX B\n"); break;
        case 0x0B: printf("DCX B\n"); break;
        case 0x0C: printf("INR C\n"); break;
        case 0x0D: printf("DCR C\n"); break;
        case 0x0E: printf("MVI C, %#02X\n", instruction[1]); break;
        case 0x0F: printf("RRC\n"); break;
        case 0x10: printf("*NOP\n"); break;
        case 0x11: printf("LXI D, %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0x12: printf("STAX D\n"); break;
        case 0x13: printf("INX D\n"); break;
        case 0x14: printf("INR D\n"); break;
        case 0x15: printf("DCR D\n"); break;
        case 0x16: printf("MVI D, %#02X\n", instruction[1]); break;
        case 0x17: printf("RAL\n"); break;
        case 0x18: printf("*NOP\n"); break;
        case 0x19: printf("DAD D\n"); break;
        case 0x1A: printf("LDAX D\n"); break;
        case 0x1B: printf("DCX D\n"); break;
        case 0x1C: printf("INR E\n"); break;
        case 0x1D: printf("DCR E\n"); break;
        case 0x1E: printf("MVI E, %#02X\n", instruction[1]); break;
        case 0x1F: printf("RAR\n"); break;
        case 0x20: printf("*NOP\n"); break;
        case 0x21: printf("LXI H, %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0x22: printf("SHLD %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0x23: printf("INX H\n"); break;
        case 0x24: printf("INR H\n"); break;
        case 0x25: printf("DCR H\n"); break;
        case 0x26: printf("MVI H, %#02X\n", instruction[1]); break;
        case 0x27: printf("DAA\n"); break;
        case 0x28: printf("*NOP\n"); break;
        case 0x29: printf("DAD H\n"); break;
        case 0x2A: printf("LHLD %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0x2B: printf("DCX H\n"); break;
        case 0x2C: printf("INR L\n"); break;
        case 0x2D: printf("DCR L\n"); break;
        case 0x2E: printf("MVI L, %#02X\n", instruction[1]); break;
        case 0x2F: printf("CMA\n"); break;
        case 0x30: printf("*NOP\n"); break;
        case 0x31: printf("LXI SP, %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0x32: printf("STA %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0x33: printf("INX SP\n"); break;
        case 0x34: printf("INR M\n"); break;
        case 0x35: printf("DCR M\n"); break;
        case 0x36: printf("MVI M, %#02X\n", instruction[1]); break;
        case 0x37: printf("STC\n"); break;
        case 0x38: printf("*NOP\n"); break;
        case 0x39: printf("DAD SP\n"); break;
        case 0x3A: printf("LDA %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0x3B: printf("DCX SP\n"); break;
        case 0x3C: printf("INR A\n"); break;
        case 0x3D: printf("DCR A\n"); break;
        case 0x3E: printf("MVI A, %#02X\n", instruction[1]); break;
        case 0x3F: printf("CMC\n"); break;
        case 0x40: printf("MOV B, B\n"); break;
        case 0x41: printf("MOV B, C\n"); break;
        case 0x42: printf("MOV B, D\n"); break;
        case 0x43: printf("MOV B, E\n"); break;
        case 0x44: printf("MOV B, H\n"); break;
        case 0x45: printf("MOV B, L\n"); break;
        case 0x46: printf("MOV B, M\n"); break;
        case 0x47: printf("MOV B, A\n"); break;
        case 0x48: printf("MOV C, B\n"); break;
        case 0x49: printf("MOV C, C\n"); break;
        case 0x4A: printf("MOV C, D\n"); break;
        case 0x4B: printf("MOV C, E\n"); break;
        case 0x4C: printf("MOV C, H\n"); break;
        case 0x4D: printf("MOV C, L\n"); break;
        case 0x4E: printf("MOV C, M\n"); break;
        case 0x4F: printf("MOV C, A\n"); break;
        case 0x50: printf("MOV D, B\n"); break;
        case 0x51: printf("MOV D, C\n"); break;
        case 0x52: printf("MOV D, D\n"); break;
        case 0x53: printf("MOV D, E\n"); break;
        case 0x54: printf("MOV D, H\n"); break;
        case 0x55: printf("MOV D, L\n"); break;
        case 0x56: printf("MOV D, M\n"); break;
        case 0x57: printf("MOV D, A\n"); break;
        case 0x58: printf("MOV E, B\n"); break;
        case 0x59: printf("MOV E, C\n"); break;
        case 0x5A: printf("MOV E, D\n"); break;
        case 0x5B: printf("MOV E, E\n"); break;
        case 0x5C: printf("MOV E, H\n"); break;
        case 0x5D: printf("MOV E, L\n"); break;
        case 0x5E: printf("MOV E, M\n"); break;
        case 0x5F: printf("MOV E, A\n"); break;
        case 0x60: printf("MOV H, B\n"); break;
        case 0x61: printf("MOV H, C\n"); break;
        case 0x62: printf("MOV H, D\n"); break;
        case 0x63: printf("MOV H, E\n"); break;
        case 0x64: printf("MOV H, H\n"); break;
        case 0x65: printf("MOV H, L\n"); break;
        case 0x66: printf("MOV H, M\n"); break;
        case 0x67: printf("MOV H, A\n"); break;
        case 0x68: printf("MOV L, B\n"); break;
        case 0x69: printf("MOV L, C\n"); break;
        case 0x6A: printf("MOV L, D\n"); break;
        case 0x6B: printf("MOV L, E\n"); break;
        case 0x6C: printf("MOV L, H\n"); break;
        case 0x6D: printf("MOV L, L\n"); break;
        case 0x6E: printf("MOV L, M\n"); break;
        case 0x6F: printf("MOV L, A\n"); break;
        case 0x70: printf("MOV M, B\n"); break;
        case 0x71: printf("MOV M, C\n"); break;
        case 0x72: printf("MOV M, D\n"); break;
        case 0x73: printf("MOV M, E\n"); break;
        case 0x74: printf("MOV M, H\n"); break;
        case 0x75: printf("MOV M, L\n"); break;
        case 0x76: printf("HALT\n"); break;
        case 0x77: printf("MOV M, A\n"); break;
        case 0x78: printf("MOV A, B\n"); break;
        case 0x79: printf("MOV A, C\n"); break;
        case 0x7A: printf("MOV A, D\n"); break;
        case 0x7B: printf("MOV A, E\n"); break;
        case 0x7C: printf("MOV A, H\n"); break;
        case 0x7D: printf("MOV A, L\n"); break;
        case 0x7E: printf("MOV A, M\n"); break;
        case 0x7F: printf("MOV A, A\n"); break;
        case 0x80: printf("ADD B\n"); break;
        case 0x81: printf("ADD C\n"); break;
        case 0x82: printf("ADD D\n"); break;
        case 0x83: printf("ADD E\n"); break;
        case 0x84: printf("ADD L\n"); break;
        case 0x85: printf("ADD H\n"); break;
        case 0x86: printf("ADD M\n"); break;
        case 0x87: printf("ADD A\n"); break;
        case 0x88: printf("ADC B\n"); break;
        case 0x89: printf("ADC C\n"); break;
        case 0x8A: printf("ADC D\n"); break;
        case 0x8B: printf("ADC E\n"); break;
        case 0x8C: printf("ADC H\n"); break;
        case 0x8D: printf("ADC L\n"); break;
        case 0x8E: printf("ADC M\n"); break;
        case 0x8F: printf("ADC A\n"); break;
        case 0x90: printf("SUB B\n"); break;
        case 0x91: printf("SUB C\n"); break;
        case 0x92: printf("SUB D\n"); break;
        case 0x93: printf("SUB E\n"); break;
        case 0x94: printf("SUB L\n"); break;
        case 0x95: printf("SUB H\n"); break;
        case 0x96: printf("SUB M\n"); break;
        case 0x97: printf("SUB A\n"); break;
        case 0x98: printf("SBB B\n"); break;
        case 0x99: printf("SBB C\n"); break;
        case 0x9A: printf("SBB D\n"); break;
        case 0x9B: printf("SBB E\n"); break;
        case 0x9C: printf("SBB H\n"); break;
        case 0x9D: printf("SBB L\n"); break;
        case 0x9E: printf("SBB M\n"); break;
        case 0x9F: printf("SBB A\n"); break;
        case 0xA0: printf("ANA B\n"); break;
        case 0xA1: printf("ANA C\n"); break;
        case 0xA2: printf("ANA D\n"); break;
        case 0xA3: printf("ANA E\n"); break;
        case 0xA4: printf("ANA H\n"); break;
        case 0xA5: printf("ANA L\n"); break;
        case 0xA6: printf("ANA M\n"); break;
        case 0xA7: printf("ANA A\n"); break;
        case 0xA8: printf("XRA B\n"); break;
        case 0xA9: printf("XRA C\n"); break;
        case 0xAA: printf("XRA D\n"); break;
        case 0xAB: printf("XRA E\n"); break;
        case 0xAC: printf("XRA H\n"); break;
        case 0xAD: printf("XRA L\n"); break;
        case 0xAE: printf("XRA M\n"); break;
        case 0xAF: printf("XRA A\n"); break;
        case 0xB0: printf("ORA B\n"); break;
        case 0xB1: printf("ORA C\n"); break;
        case 0xB2: printf("ORA D\n"); break;
        case 0xB3: printf("ORA E\n"); break;
        case 0xB4: printf("ORA H\n"); break;
        case 0xB5: printf("ORA L\n"); break;
        case 0xB6: printf("ORA M\n"); break;
        case 0xB7: printf("ORA A\n"); break;
        case 0xB8: printf("CMP B\n"); break;
        case 0xB9: printf("CMP C\n"); break;
        case 0xBA: printf("CMP D\n"); break;
        case 0xBB: printf("CMP E\n"); break;
        case 0xBC: printf("CMP H\n"); break;
        case 0xBD: printf("CMP L\n"); break;
        case 0xBE: printf("CMP M\n"); break;
        case 0xBF: printf("CMP A\n"); break;
        case 0xC0: printf("RNZ\n"); break;
        case 0xC1: printf("POP B\n"); break;
        case 0xC2: printf("JNZ %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xC3: printf("JMP %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xC4: printf("CNZ %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xC5: printf("PUSH B\n"); break;
        case 0xC6: printf("ADI %#02X\n", instruction[1]); break;
        case 0xC7: printf("RST 0\n"); break;
        case 0xC8: printf("RZ\n"); break;
        case 0xC9: printf("RET\n"); break;
        case 0xCA: printf("JZ %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xCB: printf("*JMP %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xCC: printf("CZ %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xCD: printf("CALL %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xCE: printf("ACI %#02X\n", instruction[1]); break;
        case 0xCF: printf("RST 1\n"); break;
        case 0xD0: printf("RNC\n"); break;
        case 0xD1: printf("POP D\n"); break;
        case 0xD2: printf("JNC %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xD3: printf("OUT %#02X\n", instruction[1]); break;
        case 0xD4: printf("CNC %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xD5: printf("PUSH D\n"); break;
        case 0xD6: printf("SUI %#02X\n", instruction[1]); break;
        case 0xD7: printf("RST 2\n"); break;
        case 0xD8: printf("RC\n"); break;
        case 0xD9: printf("*RET\n"); break;
        case 0xDA: printf("JC %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xDB: printf("IN %#02X\n", instruction[1]); break;
        case 0xDC: printf("CC %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xDD: printf("*CALL %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xDE: printf("SBI %#02X\n", instruction[1]); break;
        case 0xDF: printf("RST 3\n"); break;
        case 0xE0: printf("ROP\n"); break;
        case 0xE1: printf("POP H\n"); break;
        case 0xE2: printf("JPO %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xE3: printf("XTHL\n"); break;
        case 0xE4: printf("CPO %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xE5: printf("PUSH H\n"); break;
        case 0xE6: printf("ANI %#02X\n", instruction[1]); break;
        case 0xE7: printf("RST 4\n"); break;
        case 0xE8: printf("RPE\n"); break;
        case 0xE9: printf("PCHL\n"); break;
        case 0xEA: printf("JPE %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xEB: printf("XCHG\n"); break;
        case 0xEC: printf("CPE %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xED: printf("*CALL %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xEE: printf("XRI %#02X\n", instruction[1]); break;
        case 0xEF: printf("RST 5\n"); break;
        case 0xF0: printf("RP\n"); break;
        case 0xF1: printf("POP PSW\n"); break;
        case 0xF2: printf("JP %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xF3: printf("DI\n"); break;
        case 0xF4: printf("CP %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xF5: printf("PUSH PSW\n"); break;
        case 0xF6: printf("ORI %#02X\n", instruction[1]); break;
        case 0xF7: printf("RST 6\n"); break;
        case 0xF8: printf("RM\n"); break;
        case 0xF9: printf("SPHL\n"); break;
        case 0xFA: printf("JM %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xFB: printf("EI\n"); break;
        case 0xFC: printf("CM %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xFD: printf("*CALL %#04X\n", (instruction[2] << 8) | instruction[1]); break;
        case 0xFE: printf("CPI %#02X\n", instruction[1]); break;
        case 0xFF: printf("RST 7\n"); break;
        default: printf("UNKNOWN\n"); break;
    }
}
#endif

MicroprocError MicroprocInit(const char *binary_name)
{
    if(Intel8080.init)
    {
        return ALREADY_INIT;
    }
    else
    {
        memset(&Intel8080, 0, sizeof(Intel8080));
        if(SUCCESSFUL == LoadBinary(binary_name))
        {
            Intel8080.init = 1;
        }
    }
    return SUCCESSFUL;
}

MicroprocError MicroprocDeinit(void)
{
    if(!Intel8080.init)
    {
        return INVALID_STATE;
    }
    memset(&Intel8080, 0, sizeof(Intel8080));
    return SUCCESSFUL;
}

MicroprocError MicroprocRun(void)
{
    MicroprocError ret = SUCCESSFUL;
    if(!Intel8080.init)
    {
        return INVALID_STATE;
    }
    return ret;
}


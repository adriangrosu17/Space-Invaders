#ifndef _INC_INTEL8080_H_
#define _INC_INTEL8080_H_

#include <stdint.h>

#define TOTAL_MEMORY  (64 * 1024)

typedef enum
{
    SUCCESSFUL = 0,
    INVALID_BINARY,
    MEMORY_ERROR,
    IO_ERROR,
    ALREADY_INIT,
    INVALID_STATE,
    NULL_PARAMETER,
}MicroprocError;

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
}Microproc;

MicroprocError MicroprocInit(Microproc *Intel8080, const char *binary_name);
MicroprocError MicroprocDeinit(Microproc *Intel8080);
MicroprocError MicroprocStep(Microproc *Intel8080);

#endif // _INC_INTEL8080_H_


#ifndef _INC_INTEL8080_H_
#define _INC_INTEL8080_H_

#include <stdint.h>

typedef enum
{
    SUCCESSFUL = 0,
    INVALID_BINARY,
    MEMORY_FAIL,
    IO_ERROR,
    ALREADY_INIT,
    INVALID_STATE,
}MicroprocError;

MicroprocError MicroprocInit(const char *binary_name);
MicroprocError MicroprocDeinit(void);
MicroprocError MicroprocRun(void);

#endif // _INC_INTEL8080_H_


#include <stdio.h>
#include <intel8080.h>
#include <assert.h>

static Microproc Intel8080 = {0};

int main(int argc, char *argv[])
{
    if(argc != 2)
    {
        printf("Invalid number of arguments provided\n");
        exit(-1);
    }
    MicroprocError sc = SUCCESSFUL;
    sc = MicroprocInit(&Intel8080, argv[1]);
    assert("Microproc init failed" && SUCCESSFUL == sc);
    sc = MicroprocDeinit(&Intel8080);
    assert("Microproc deinit failed" && SUCCESSFUL == sc);
    sc = MicroprocInit(&Intel8080, argv[1]);
    assert("Microproc reinit failed" && SUCCESSFUL == sc);
    sc = MicroprocStep(&Intel8080);
    assert("Microproc step failed" && SUCCESSFUL == sc);
    return 0;
}

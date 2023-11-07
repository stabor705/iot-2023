#include "mbed.h"
#include "TMP102.h"

// main() runs in its own thread in the OS
int main()
{
    TMP102 tmp(D14, D15, 0x90);
    while (true) {
        printf("%.3f", tmp.read_12b());
    } 
}


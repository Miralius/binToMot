//
// Created by F-Mir on 3/26/2024.
//
#include "binToMot.h"

int main()
{
    Params params;
    params.end_addr = 0x43FF;
    params.input_filename = "CEM_DONOR_FULL.bin";
    params.output_filename = "output.mot";
    params.do_headers = false;
    return binToMot(params);
}
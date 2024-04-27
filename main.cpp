//
// Created by F-Mir on 3/26/2024.
//
#include <iostream>
#include <cassert>
#include <filesystem>
#include "binToMot.h"

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[]) {
    Params params;
    if (argc > 1 and strcmp(argv[1], "-debug") == 0) {
        params.input_filename = "CEM_DONOR_FULL.bin";
        params.output_filename = "output.mot";
    } else {
        std::cout << "Enter .bin file: ";
        std::cin >> params.input_filename;
        std::cout << "Enter .mot file: ";
        std::cin >> params.output_filename;
    }
    params.do_headers = false;
    params.appendingMode = false;
    params.isTheLastBlock = false;
    const auto max_addr = std::filesystem::file_size(params.input_filename) - 1;
    params.begin_addr = 0x0;
    params.end_addr = 0xFFFF;
    bool fileFinished = false;
    while (!fileFinished) {
        if (params.end_addr == max_addr)
        {
            params.isTheLastBlock = true;
        }
        int result = binToMot(params);
        assert(result == 0);
        if (!params.appendingMode) {
            params.appendingMode = true;
        }
        params.begin_addr += 0x10000;
        if (params.begin_addr == 0x180000)
        {
            params.begin_addr = 0x800000;
            params.end_addr = 0x80FFFF;
        }
        else
        {
            if (params.end_addr != max_addr) {
                params.end_addr = std::min<uint32_t>(params.end_addr + 0x10000, max_addr);
            } else {
                fileFinished = true;
            }
        }
    }

    return 0;
}
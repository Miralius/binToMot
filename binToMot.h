/*  
    BIN2SREC  - Convert binary to Motorola S-Record file  
    Copyright (C) 1998-2012  Anthony Goffart  
  
    This program is free software: you can redistribute it and/or modify  
    it under the terms of the GNU General Public License as published by  
    the Free Software Foundation, either version 3 of the License, or  
    (at your option) any later version.  
  
    This program is distributed in the hope that it will be useful,  
    but WITHOUT ANY WARRANTY; without even the implied warranty of  
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the  
    GNU General Public License for more details.  
  
    You should have received a copy of the GNU General Public License  
    along with this program.  If not, see <http://www.gnu.org/licenses/>.  
*/

#include <cstdio>
#include <cstring>
#include <cstdint>
#include <sys/stat.h>
#include <string>
#include <optional>

#define SREC_VER "1.43"

#define HEADER2 "Copyright (c) 2000-2014 Ant Goffart - http://www.s-record.com/\n\n"

#define HEADER1 "\nBIN2SREC " SREC_VER " - Convert binary to Motorola S-Record file.\n"

std::string input_filename;
FILE *infile;
std::string output_filename;
FILE *outfile;

uint32_t addr_offset = 0;
uint32_t begin_addr;
uint32_t end_addr;
int addr_bytes = 2;
int do_headers = true;
int verbose = true;
uint32_t line_length = 32;

struct Params
{
    std::optional<uint32_t> addr_offset;
    std::optional<uint32_t> begin_addr;
    std::optional<uint32_t> end_addr;
    std::optional<int> addr_bytes;
    std::optional<uint32_t> line_length;
    std::optional<bool> do_headers;
    std::optional<bool> verbose;
    std::string input_filename;
    std::string output_filename;
};

/***************************************************************************/

uint32_t file_size(FILE *f) {
    struct stat info{};

    if (!fstat(_fileno(f), &info))
        return ((uint32_t) info.st_size);
    else
        return (0);
}

/***************************************************************************/

void syntax() {
    fprintf(stderr, HEADER1);
    fprintf(stderr, HEADER2);
    fprintf(stderr, "Syntax: BIN2SREC <options> INFILE > OUTFILE\n\n");
    fprintf(stderr, "-help            Show this help.\n");
    fprintf(stderr, "-b <begin>       Address to begin at in binary file (hex), default = 0.\n");
    fprintf(stderr, "-e <end>         Address to end at in binary file (hex), default = end of file.\n");
    fprintf(stderr, "-o <offset>      Generated address offset (hex), default = begin address.\n");
    fprintf(stderr, "-a <addrsize>    Number of bytes used for address (2-4),\n");
    fprintf(stderr, "                  default = minimum needed for maximum address.\n");
    fprintf(stderr, "-l <linelength>  Number of bytes per line (8-32), default = 32.\n");
    fprintf(stderr, "-s               Suppress header and footer records.\n");
    fprintf(stderr, "-q               Quiet mode - no output except S-Record.\n");
}

/***************************************************************************/

void process() {
    int i;
    uint32_t max_addr, address;
    uint32_t byte_count;
    uint32_t this_line;
    unsigned char checksum;
    uint32_t c;
    int record_count = 0;

    unsigned char buf[32];

    max_addr = addr_offset + (end_addr - begin_addr);

    fseek(infile, static_cast<int>(begin_addr), SEEK_SET);

    if ((max_addr > 0xffffl) && (addr_bytes < 3))
        addr_bytes = 3;

    if ((max_addr > 0xffffffl) && (addr_bytes < 4))
        addr_bytes = 4;

    if (verbose) {
        fprintf(stderr, HEADER1);
        fprintf(stderr, HEADER2);
        fprintf(stderr, "Input binary file:  %s\n", input_filename.c_str());
        fprintf(stderr, "Output binary file: %s\n", output_filename.c_str());
        fprintf(stderr, "Begin address     = %Xh\n", begin_addr);
        fprintf(stderr, "End address       = %Xh\n", end_addr);
        fprintf(stderr, "Address offset    = %Xh\n", addr_offset);
        fprintf(stderr, "Maximum address   = %Xh\n", max_addr);
        fprintf(stderr, "Address bytes     = %d\n", addr_bytes);
    }

    if (do_headers)
        fprintf(outfile, "S00600004844521B\n");       /* Header record */

    address = addr_offset;

    for (;;) {
        if (verbose)
            fprintf(stderr, "Processing %08Xh\r", address);

        this_line = std::min(line_length, (max_addr - address) + 1);
        byte_count = (addr_bytes + this_line + 1);
        fprintf(outfile, "S%d%02X", addr_bytes - 1, byte_count);

        checksum = byte_count;

        for (i = addr_bytes - 1; i >= 0; i--) {
            c = (address >> (i << 3)) & 0xff;
            fprintf(outfile, "%02X", c);
            checksum += c;
        }

        fread(buf, 1, this_line, infile);

        for (i = 0; i < this_line; i++) {
            fprintf(outfile, "%02X", buf[i]);
            checksum += buf[i];
        }

        fprintf(outfile, "%02X\n", 255 - checksum);

        record_count++;

        /* check before adding to allow for finishing at 0xffffffff */
        if ((address - 1 + line_length) >= max_addr)
            break;

        address += line_length;
    }

    if (do_headers) {
        if (record_count > 0xffff) {
            checksum = 4 + (record_count & 0xff) + ((record_count >> 8) & 0xff) + ((record_count >> 16) & 0xff);
            fprintf(outfile, "S604%06X%02X\n", record_count, 255 - checksum);
        } else {
            checksum = 3 + (record_count & 0xff) + ((record_count >> 8) & 0xff);
            fprintf(outfile, "S503%04X%02X\n", record_count, 255 - checksum);
        }

        byte_count = (addr_bytes + 1);
        fprintf(outfile, "S%d%02X", 11 - addr_bytes, byte_count);

        checksum = byte_count;

        for (i = addr_bytes - 1; i >= 0; i--) {
            c = (addr_offset >> (i << 3)) & 0xff;
            fprintf(outfile, "%02X", c);
            checksum += c;
        }
        fprintf(outfile, "%02X\n", 255 - checksum);
    }

    if (verbose)
        fprintf(stderr, "Processing complete \n");
}

/***************************************************************************/

int binToMot(const Params& params) {

    if (params.input_filename.empty() or params.output_filename.empty()) {
        syntax();
        fprintf(stderr, "\n** No input/output filename specified\n");
        return (1);
    }

    if (fopen_s(&infile, input_filename.c_str(), "rb") == NULL
    and fopen_s(&outfile, output_filename.c_str(), "w") == NULL) {
        uint32_t size = file_size(infile) - 1;
        begin_addr = params.begin_addr ? *params.begin_addr : 0u;
        addr_offset = params.addr_offset ? *params.addr_offset : begin_addr;
        end_addr = params.end_addr ? std::min(size, *params.end_addr) : size;
        if (params.addr_bytes)
        {
            if (*params.addr_bytes > 4)
            {
                addr_bytes = 4;
            }
            else if (*params.addr_bytes < 2)
            {
                addr_bytes = 2;
            }
            else
            {
                addr_bytes = *params.addr_bytes;
            }
        }
        if (params.line_length)
        {
            if (*params.line_length > 32)
            {
                line_length = 32;
            }
            else if (*params.line_length < 8)
            {
                line_length = 8;
            }
            else
            {
                line_length = *params.line_length;
            }
        }
        do_headers = !params.do_headers || *params.do_headers;
        verbose = !params.verbose || *params.verbose;

        if (begin_addr > size) {
            fprintf(stderr, "Begin address %Xh is greater than file size %Xh\n", begin_addr, size);
            return (3);
        }

        if (end_addr < begin_addr) {
            fprintf(stderr, "End address %Xh is less than begin address %Xh\n", end_addr, begin_addr);
            return (3);
        }

        process();
        fclose(infile);
        return (0);
    } else {
        fprintf(stderr, "Input file %s not found\n", input_filename.c_str());
        return (2);
    }
}
/***************************************************************************/  

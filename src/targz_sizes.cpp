/*
 ============================================================================
 Name        : targz_sizes.cpp
 Author      : Tomasz Melcer
 Version     :
 Copyright   : GPL v3
 Description : Compute compressed sizes of individual files in a .tar.gz file
 ============================================================================
 */

#include <iostream>
#include <cstdio>
#include <zlib.h>

using namespace std;

#define COMPRESSED_BUFFER 102400

struct tarheader {
    char filename[100];
    char somejunk[24];
    char size[12];
    char otherjunk[512-100-24-12];
};

long long decode_octal(char* size) {
    long long value = 0;
    for (int i=0; i<11; i++) {
        value = value*8 + (size[i]&7);
    }
    return value;
}

int main(int argc, char** argv) {
    // argument parsing
    if (argc != 2) {
        cout << "Usage:" << endl
             << "   " << argv[0] << " file.tar.gz" << endl;

        return 1;
    }

    char* filename = argv[1];

    // opening files and setting up counters
    std::cout << "open " << filename << endl;
    FILE* file = fopen(filename, "rb");  // TODO: error checking

    unsigned char compressed[COMPRESSED_BUFFER];
    tarheader decompressed;
    gz_header gzip_header = {0};
    z_stream gzip_stream = {0};
    long long int bytes_read = 0;

#define ERROR " errmsg: " << (gzip_stream.msg ? gzip_stream.msg : "none") << endl

    gzip_stream.next_in = compressed;
    gzip_stream.avail_in = 0;
    gzip_stream.zalloc = Z_NULL;
    gzip_stream.zfree = Z_NULL;
    gzip_stream.next_out = (unsigned char*) &decompressed;
    gzip_stream.avail_out = sizeof(decompressed);
    std::cout << "inflateInit: " << inflateInit2(&gzip_stream, 15+32) << ERROR;  // TODO: error checking
    std::cout << "inflateGetHeader: " << inflateGetHeader(&gzip_stream, &gzip_header) << ERROR;
    //std::cout << "tar block size: " << sizeof(decompressed) << endl;

    // state machine
    int skip_blocks = 0;
    while(true) {
        //std::cout << "new cycle\n";
        // if stream's avail_in is empty, we need more data.
        if (gzip_stream.avail_in == 0) {
            //std::cout << "Empty input." << endl;
            // TODO: error checking
            // TODO: end of file condition
            size_t bytes = fread(&compressed, 1, COMPRESSED_BUFFER, file);
            //std::cout << "Read " << bytes << " bytes.\n";
            bytes_read += bytes;
            gzip_stream.next_in = compressed;
            gzip_stream.avail_in = bytes;
        }
        // post-condition: gzip_stream.avail_in > 0

        // if output buffer is not full, try to decompress more data
        if (gzip_stream.avail_out > 0) {
            //std::cout << "free space for decompression: " << gzip_stream.avail_out << endl;
            // TODO: error checking
            // TODO: end of stream condition
            int cond;
            std::cout << "inflate: " << (cond = inflate(&gzip_stream, Z_NO_FLUSH)) << ERROR;
            if (cond == 1)
                return 0;
        }

        // if output buffer is full, perform an action.
        if (gzip_stream.avail_out == 0) {
            //std::cout << "Full buffer. ";
            if (skip_blocks > 0) {
                decompressed.filename[20] = 0;
                //std::cout << "skipped: [[[" << (char*) &decompressed << "]]]" << endl;
                // we're inside a file. just ignore the block
                skip_blocks--;
            } else {
                // we've got a header. print details.
                // TODO: handle long file names.
                std::cout << "File: " << decompressed.filename
                          << ", decompressed size: " << decompressed.size
                          << ", position: " << gzip_stream.total_in
                          << endl;

                long long size = decode_octal(decompressed.size);
                skip_blocks = ((size + 511) / 512);

                //std::cout << "Blocks to skip: " << skip_blocks << " (from: " << size << ")" << endl;
            }

            gzip_stream.next_out = (unsigned char*) &decompressed;
            gzip_stream.avail_out = sizeof(decompressed);
        }
    }
}

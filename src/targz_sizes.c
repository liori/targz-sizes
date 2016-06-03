/*
 vim: ts=4 sw=4 et
 ============================================================================
 Name        : targz_sizes.c
 Author      : Tomasz Melcer
 Version     :
 License     : GPL v3
 Description : Compute compressed sizes of individual files in a .tar.gz file
 ============================================================================
 */

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>

#include <zlib.h>

#define COMPRESSED_BUFFER 102400
#define TAR_BLOCK_SIZE 512

#ifndef max
    #define max( a, b ) ( ((a) > (b)) ? (a) : (b) )
#endif

#ifndef min
    #define min( a, b ) ( ((a) < (b)) ? (a) : (b) )
#endif

char* arg0;
#define LOG_INIT(A) arg0=A
#define LOG_MESSAGE(T,N,L,M, ...) \
    if ( L >= T ) { \
        if ( L >= 3 ) \
            fprintf(stderr, "[%s] (%s:%d) " M "\n", N, \
                    __FILE__, __LINE__, ##__VA_ARGS__ ) ; \
        else \
            fprintf(stderr, "%s: " M "\n", arg0, ##__VA_ARGS__ ) ; \
    }

#define LOG_ERROR(L,M, ...) LOG_MESSAGE(1,"error",L,M,##__VA_ARGS__)
#define LOG_WARN(L,M, ...) LOG_MESSAGE(2,"warn",L,M,##__VA_ARGS__)
#define LOG_INFO(L,M, ...) LOG_MESSAGE(3,"info",L,M,##__VA_ARGS__)
#define LOG_DEBUG(L,M, ...) LOG_MESSAGE(4,"debug",L,M,##__VA_ARGS__)
#define LOG_TEST(L,M, ...) LOG_MESSAGE(5,"test",L,M,##__VA_ARGS__)

struct tarheader {
    char filename[100];
    char somejunk[24];
    char size[12];
    char otherjunk[512-100-24-12];
};

// technically 36 bits should be enough, but there is no uint_least36_t type
typedef uint_least64_t tarfilesize_t;

tarfilesize_t decode_octal(char* size) {
    tarfilesize_t value = 0;
    for (int i=0; i<11; i++) {
        value = value*8 + (size[i]&7);
    }
    return value;
}

int main(int argc, char** argv) {
    // argument parsing

    LOG_INIT(argv[0]);

    int c;

    int verbosity=0;

    while (1) {
        static struct option long_options[] = {
            {"log-level", required_argument, 0, 'v'},
            {0, 0, 0, 0}
        };
        // getopt_long stores the option index here
        int option_index = 0;

        c = getopt_long (
            argc, argv, "v:",
            long_options, &option_index
        );

        // detect the end of the options
        if (c == -1)
            break;

        switch (c) {
            case 0:
            // coding error, this case should only be found
            // if the long_options struct is set up with flag
            // pointers, which in the current implementation
            // it is not.
            LOG_ERROR(verbosity, "coding error, getopt_long() should not "
                    "return '0' based on expected long_options input" );
            break;

            case 'v':
            verbosity=max(0, min(5, atoi(optarg)));
            LOG_INFO(verbosity, "logging level %d enabled", verbosity );
            break;

            case '?':
            // getopt_long already printed an error message
            exit(EXIT_FAILURE);
            break;

            default:
            abort ();
        }
    }

    unsigned char compressed[COMPRESSED_BUFFER];
    unsigned char decompressed[TAR_BLOCK_SIZE];
    struct tarheader header;
    gz_header gzip_header = {0};
    z_stream gzip_stream = {0};
    long long int start_of_last_header = 0;

    gzip_stream.next_in = compressed;
    gzip_stream.avail_in = 0;
    gzip_stream.zalloc = Z_NULL;
    gzip_stream.zfree = Z_NULL;
    gzip_stream.next_out = (unsigned char*) &header;
    gzip_stream.avail_out = sizeof(header);
    inflateInit2(&gzip_stream, 15+32); // TODO: error checking
    inflateGetHeader(&gzip_stream, &gzip_header);

    // state machine
    int skip_blocks = 0;
    int return_code = 0;
    int more_data = 1;
    while(more_data) {
        // if stream's avail_in is empty, we need more data.
        if (gzip_stream.avail_in == 0) {
            size_t bytes = fread(&compressed, 1, COMPRESSED_BUFFER, stdin);
            if (bytes == 0 && ferror(stdin)) {
                perror("stdin");
                return_code = 1;
                break;
            } else if (bytes == 0 && feof(stdin)) {
                fprintf(stderr, "stdin: premature end of file\n");
                return_code = 1;
                break;
            }
            gzip_stream.next_in = compressed;
            gzip_stream.avail_in = bytes;
        }
        // post-condition: gzip_stream.avail_in > 0

        // if output buffer is not full, try to decompress more data
        if (gzip_stream.avail_out > 0) {
            int cond = inflate(&gzip_stream, Z_NO_FLUSH);
            if (cond == Z_STREAM_END) {
                // this is the last iteration of the loop
                more_data = 0;
            } else if (cond < 0) {
                fprintf(stderr, "stdin: %s\n", gzip_stream.msg);
                return_code = 1;
                break;
            }
        }

        // if output buffer is full, perform an action.
        if (gzip_stream.avail_out == 0) {
            if (skip_blocks > 0) {
                // we're inside a file. just ignore the block
                skip_blocks--;
            } else {
                // we've got a header
                tarfilesize_t size = decode_octal(header.size);
                skip_blocks = ((size + 511) / 512);
            }

            if (skip_blocks > 0) {
                gzip_stream.next_out = (unsigned char*) &decompressed;
                gzip_stream.avail_out = sizeof(decompressed);
            } else {
                // tar uses blocks with null filename to mark end of stream.
                // ignore them.
                // TODO: handle long file names.
                if (header.filename[0] != 0) {
                    // to ensure the filename is 0-terminated somewhere (note:
                    // we're not handling longer filenames than those which
                    // fit the original tar limits).
                    header.somejunk[0] = 0;
                    printf("%lld %s\n",
                        gzip_stream.total_in - start_of_last_header,
                        header.filename);
                }

                start_of_last_header = gzip_stream.total_in;

                gzip_stream.next_out = (unsigned char*) &header;
                gzip_stream.avail_out = sizeof(header);
            }
        }
    }

    // free resources and make valgrind happy
    inflateEnd(&gzip_stream);

    return return_code;
}

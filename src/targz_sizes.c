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

static inline int max(int a, int b) {
  return a > b ? a : b;
}

static inline int min(int a, int b) {
  return a > b ? a : b;
}

char* arg0;
#define LOG_INIT(A) arg0 = A
#define LOG_MESSAGE(T,N,L,M, ...) \
    if (L >= T) { \
        if (L >= 3) \
            fprintf(stderr, "[%s] (%s:%d) " M "\n", N, \
                    __FILE__, __LINE__, ##__VA_ARGS__) ; \
        else \
            fprintf(stderr, "%s: " M "\n", arg0, ##__VA_ARGS__) ; \
    }

#define LOG_ERROR(L,M, ...) LOG_MESSAGE(1,"error",L,M,##__VA_ARGS__)
#define LOG_WARN(L,M, ...) LOG_MESSAGE(2,"warn",L,M,##__VA_ARGS__)
#define LOG_INFO(L,M, ...) LOG_MESSAGE(3,"info",L,M,##__VA_ARGS__)
#define LOG_DEBUG(L,M, ...) LOG_MESSAGE(4,"debug",L,M,##__VA_ARGS__)
#define LOG_TEST(L,M, ...) LOG_MESSAGE(5,"test",L,M,##__VA_ARGS__)

#define LEVEL_IS_TEST(L) L >= 5

struct tarheader {
    char filename[100];
    char somejunk[24];
    char size[12];
    char morejunk[20];
    char typeflag;
    char otherjunk[512-100-24-12-20-1];
};

// technically 36 bits should be enough, but there is no uint_least36_t type
typedef uint_least64_t tarfilesize_t;

tarfilesize_t decode_octal(char* size) {
    tarfilesize_t value = 0;
    for (int i = 0; i < 11; i++) {
        value = value*8 + (size[i]&7);
    }
    return value;
}

typedef char tarblock_t[TAR_BLOCK_SIZE];

int main(int argc, char** argv) {
    // argument parsing

    LOG_INIT(argv[0]);

    int c;

    int verbosity = 0;
    int separator = '\n';

    while (1) {
        static struct option long_options[] = {
            {"log-level", required_argument, 0, 'v'},
            {"null", no_argument, 0, 'Z'},
            {0, 0, 0, 0}
        };
        // getopt_long stores the option index here
        int option_index = 0;

        c = getopt_long (
            argc, argv, "Zv:",
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
                    "return '0' based on expected long_options input");
            break;

            case 'Z':
            separator = '\0';
            LOG_INFO(verbosity, "null separator enabled");
            break;

            case 'v':
            verbosity = max(0, min(5, atoi(optarg)));
            LOG_INFO(verbosity, "logging level %d enabled", verbosity);
            break;

            case '?':
            // getopt_long already printed an error message
            exit(EXIT_FAILURE);
            break;

            default:
            abort ();
        }
    }

    unsigned int max_filename_blocks;
    if (LEVEL_IS_TEST(verbosity)) {
        // when targz_sizes is in *test* mode, set max_filename_blocks
        // very small (2).  This makes it easier to test the case where
        // a long filename would overrun the buffer
        max_filename_blocks = 2;
    } else {
        // normally we set max_filename_blocks large
        max_filename_blocks = 200;
    }
    LOG_INFO(verbosity, "max_filename_blocks = %d", max_filename_blocks);

    unsigned char compressed[COMPRESSED_BUFFER];
    unsigned char decompressed[TAR_BLOCK_SIZE];
    unsigned char filename_buff[max_filename_blocks * TAR_BLOCK_SIZE];
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
    int file_blocks = 0;
    int skip_blocks = 0;
    int read_blocks = 0;
    unsigned char* long_filename = filename_buff;
    tarfilesize_t file_size = 0;
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
            if (read_blocks > 0) {
                // we're inside an extended filename. read the block
                LOG_DEBUG(verbosity, "read a data block");
                read_blocks--;
            } else if (skip_blocks > 0) {
                // we're inside a file. just ignore the block
                LOG_DEBUG(verbosity, "skipped a data block");
                skip_blocks--;
            } else {
                // we've got a header
                file_size = decode_octal(header.size);
                // to ensure the filename is 0-terminated somewhere
                header.somejunk[0] = 0;
                LOG_DEBUG(verbosity, "read a header block, typeflag = '%c', "
                        "data size = %llu, filename = \"%s\".", header.typeflag,
                        file_size, header.filename);
                file_blocks = ((file_size + 511) / 512);
                if (header.typeflag == 'L') {
                    if (file_blocks < max_filename_blocks) {
                        skip_blocks = 0;
                        read_blocks = file_blocks;
                    } else {
                        skip_blocks = file_blocks - max_filename_blocks;
                        read_blocks = file_blocks = max_filename_blocks;
                        file_size = max_filename_blocks * TAR_BLOCK_SIZE - 1;
                    }
                    LOG_TEST(verbosity, "read_blocks = %d", read_blocks);
                    LOG_TEST(verbosity, "skip_blocks = %d", skip_blocks);
                } else {
                    skip_blocks = file_blocks;
                    read_blocks = 0;
                }
            }

            if (read_blocks > 0) {
                gzip_stream.next_out = &(filename_buff[
                        (file_blocks - read_blocks) * TAR_BLOCK_SIZE]);
                gzip_stream.avail_out = sizeof(tarblock_t);
            } else if (skip_blocks > 0) {
                gzip_stream.next_out = (unsigned char*) &decompressed;
                gzip_stream.avail_out = sizeof(decompressed);
            } else {
                // tar uses blocks with null filename to mark end of stream.
                // ignore them.
                if (header.filename[0] != 0) {
                    if (header.typeflag == 'L') {
                        // ensure the filename_buff is 0-terminated somewhere
                        filename_buff[
                        (max_filename_blocks * TAR_BLOCK_SIZE) - 1] = 0;
                    } else {
                        if (long_filename[0]) {
                            printf("%lld %s%c",
                                gzip_stream.total_in - start_of_last_header,
                                long_filename, separator);
                            long_filename[0] = 0;
                        } else {
                            printf("%lld %s%c",
                                gzip_stream.total_in - start_of_last_header,
                                header.filename, separator);
                        }
                    }
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

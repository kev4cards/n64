#include <stdlib.h>
#include <string.h>
#ifdef N64_MACOS
#include <limits.h>
#else
#include <linux/limits.h>
#endif
#include <bzlib.h>
#include <system/n64system.h>
#include <cpu/rsp.h>
#include <mem/mem_util.h>
#include <cpu/n64_rsp_bus.h>

// Just to make sure we don't get caught in an infinite loop
#define MAX_CYCLES 100000

void load_rsp_imem(const char* rsp_path) {
    FILE* rsp = fopen(rsp_path, "rb");
    size_t read = fread(N64RSP.sp_imem, 1, SP_IMEM_SIZE, rsp);

    // File is in big endian, byte swap it all.
    for (int i = 0; i < SP_IMEM_SIZE; i += 4) {
        u32 instr = word_from_byte_array((u8*) &N64RSP.sp_imem, i);
        instr = be32toh(instr);
        word_to_byte_array((u8*) &N64RSP.sp_imem, i, instr);
    }

    if (read == 0) {
        logfatal("Read 0 bytes from %s", rsp_path);
    }

    for (int i = 0; i < SP_IMEM_SIZE / 4; i++) {
        N64RSP.icache[i].instruction.raw = word_from_byte_array(N64RSP.sp_imem, i * 4);
        N64RSP.icache[i].handler = cache_rsp_instruction;
    }
}

void load_rsp_dmem(u32* input, int input_size) {
    for (int i = 0; i < input_size; i++) {
        n64_rsp_write_word(i * 4, input[i]);
    }
}

bool run_test(u32* input, int input_size, u32* output, int output_size) {
    load_rsp_dmem(input, input_size / 4);

    N64RSP.status.halt = false;
    N64RSP.pc = 0;

    int cycles = 0;

    while (!N64RSP.status.halt) {
        if (cycles >= MAX_CYCLES) {
            logfatal("Test ran too long and was killed! Possible infinite loop?");
        }

        cycles++;
        rsp_step();
    }

    bool failed = false;
    printf("\n\n================= Expected =================    ================== Actual ==================\n");
    printf("          0 1 2 3  4 5 6 7  8 9 A B  C D E F              0 1 2 3  4 5 6 7  8 9 A B  C D E F\n");
    for (int i = 0; i < output_size; i += 16) {
        printf("0x%04X:  ", 0x800 + i);

        for (int b = 0; b < 16; b++) {
            if (b != 0 && b % 4 == 0) {
                printf(" ");
            }
            if (i + b < output_size) {
                printf("%02X", ((u8*)output)[i + b]);
            } else {
                printf("  ");
            }
        }

        printf("    0x%04X:  ", 0x800 + i);

        for (int b = 0; b < 16; b++) {
            if (b != 0 && b % 4 == 0) {
                printf(" ");
            }
            if (i + b < output_size) {
                u8 actual = N64RSP.sp_dmem[BYTE_ADDRESS(0x800 + i + b)];
                u8 expected = ((u8*)output)[i + b];

                if (actual != expected) {
                    printf(COLOR_RED);
                    failed = true;
                }
                printf("%02X", actual);
                if (actual != expected) {
                    printf(COLOR_END);
                }
            } else {
                printf("  ");
            }
        }

        printf("\n");
    }
    printf("          0 1 2 3  4 5 6 7  8 9 A B  C D E F              0 1 2 3  4 5 6 7  8 9 A B  C D E F\n");

    printf("\n\n");

    return failed;
}

void load_test(const char* rsp_path) {
    init_n64system(NULL, false, false, UNKNOWN_VIDEO_TYPE, false);
    load_rsp_imem(rsp_path);
}

int main(int argc, char** argv) {
    if (argc < 5) {
        logfatal("Not enough arguments");
    }

    log_set_verbosity(LOG_VERBOSITY_DEBUG);

    const char* test_name = argv[1];
    int input_size = atoi(argv[2]);
    int output_size = atoi(argv[3]);

    if (input_size % 4 != 0) {
        logfatal("Invalid input size: %d is not divisible by 4.", input_size);
    }

    if (output_size % 4 != 0) {
        logfatal("Invalid output size: %d is not divisible by 4.", output_size);
    }


    char input_data_path[PATH_MAX];
    snprintf(input_data_path, PATH_MAX, "%s.input", test_name);
    FILE* input_data_handle = fopen(input_data_path, "rb");

    char output_data_path[PATH_MAX];
    snprintf(output_data_path, PATH_MAX, "%s.golden", test_name);
    FILE* output_data_handle = fopen(output_data_path, "rb");


    bool failed = false;

    char rsp_path[PATH_MAX];
    snprintf(rsp_path, PATH_MAX, "%s.rsp", test_name);

    load_test(rsp_path);

    for (int i = 4; i < argc; i++) {
        const char* subtest_name = argv[i];
        u8 input[input_size];
        fread(input, 1, input_size, input_data_handle);
        u8 output[output_size];
        fread(output, 1, output_size, output_data_handle);

        bool subtest_failed = run_test((u32 *) input, input_size, (u32 *) output, output_size);

        if (subtest_failed) {
            printf("[%s %s] FAILED\n", test_name, subtest_name);
        } else {
            printf("[%s %s] PASSED\n", test_name, subtest_name);
        }

        failed |= subtest_failed;
        if (failed) {
            break;
        }
    }

    exit(failed);
}

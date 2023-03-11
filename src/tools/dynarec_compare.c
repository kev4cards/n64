#include <stdio.h>
#include <log.h>
#include <system/n64system.h>
#include <mem/pif.h>
#include <rdp/parallel_rdp_wrapper.h>
#include <imgui/imgui_ui.h>
#include <frontend/frontend.h>
#include <system/scheduler.h>
#include <disassemble.h>
#include <mem/n64bus.h>
#include <cpu/dynarec/dynarec.h>
#include <rsp.h>
#include <dynarec/v2/v2_compiler.h>
#include <sys/shm.h>
#include <sys/msg.h>

r4300i_t* n64cpu_interpreter_ptr;
int mq_jit_to_interp_id = -1;
int mq_interp_to_jit_id = -1;

struct num_cycles_msg {
    long mtype;
    int cycles;
};

void send_cycles(int id, int cycles) {
    static struct num_cycles_msg m;
    m.mtype = 1;
    m.cycles = cycles;
    if (msgsnd(id, &m, sizeof(int), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }
}

int recv_cycles(int id) {
    static struct num_cycles_msg m;
    if (msgrcv(id, &m, sizeof(m.cycles), 0, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }
    return m.cycles;
}

int create_and_configure_mq(key_t key) {
    printf("Creating an mq with key %08X\n", key);
    int mq_id = msgget(key, IPC_CREAT | 0777);
    if (mq_id == -1) {
        perror("msgget");
        exit(1);
    }
    struct msqid_ds mq_config;
    if (msgctl(mq_id, IPC_STAT, &mq_config) == -1) {
        perror("msgctl stat");
        exit(1);
    }
    mq_config.msg_qbytes = sizeof(int) + 1;
    if (msgctl(mq_id, IPC_SET, &mq_config) == -1) {
        perror("msgctl set");
        exit(1);
    }
    return mq_id;
}


bool compare() {
    bool good = true;
    good &= n64cpu_interpreter_ptr->pc == N64CPU.pc;
    for (int i = 0; i < 32; i++) {
        good &= n64cpu_interpreter_ptr->gpr[i] == N64CPU.gpr[i];
        good &= n64cpu_interpreter_ptr->f[i].raw == N64CPU.f[i].raw;
    }
    //good &= memcmp(n64sys_interpreter.mem.rdram, n64sys_dynarec.mem.rdram, N64_RDRAM_SIZE) == 0;
    return good;
}

void print_colorcoded_u64(const char* name, u64 expected, u64 actual) {
    printf("%4s 0x%016lX 0x", name, expected);
    for (int offset = 56; offset >= 0; offset -= 8) {
        u64 good_byte = (expected >> offset) & 0xFF;
        u64 bad_byte = (actual >> offset) & 0xFF;
        printf("%s%02X%s", good_byte == bad_byte ? "" : COLOR_RED, (u8)bad_byte, good_byte == bad_byte ? "" : COLOR_END);
    }
    printf("%s" COLOR_END "\n", expected == actual ? COLOR_GREEN " OK!" : COLOR_RED " BAD!");
}

void print_state() {
    printf("expected (interpreter)  actual (dynarec)\n");
    print_colorcoded_u64("PC", n64cpu_interpreter_ptr->pc, N64CPU.pc);
    printf("\n");
    for (int i = 0; i < 32; i++) {
        print_colorcoded_u64(register_names[i], n64cpu_interpreter_ptr->gpr[i], N64CPU.gpr[i]);
    }
    printf("\n");
    for (int i = 0; i < 32; i++) {
        print_colorcoded_u64(cp1_register_names[i], n64cpu_interpreter_ptr->f[i].raw, N64CPU.f[i].raw);
    }

    /*
    for (int i = 0; i < N64_RDRAM_SIZE; i++) {
        if (n64sys_interpreter.mem.rdram[i] != n64sys_dynarec.mem.rdram[i]) {
            printf("%08X: %02X %02X\n", i, n64sys_interpreter.mem.rdram[i], n64sys_dynarec.mem.rdram[i]);
        }
    }
    */
}

void copy_to(n64_system_t* sys, r4300i_t* cpu, rsp_t* rsp, scheduler_t* scheduler) {
    memcpy(sys, &n64sys, sizeof(n64_system_t));
    memcpy(cpu, n64cpu_ptr, sizeof(r4300i_t));
    memcpy(rsp, &N64RSP, sizeof(rsp_t));
    memcpy(scheduler, &n64scheduler, sizeof(scheduler_t));
}

void restore_from(n64_system_t* sys, r4300i_t* cpu, rsp_t* rsp, scheduler_t* scheduler) {
    memcpy(&n64sys, sys, sizeof(n64_system_t));
    memcpy(n64cpu_ptr, cpu, sizeof(r4300i_t));
    memcpy(&N64RSP, rsp, sizeof(rsp_t));
    memcpy(&n64scheduler, scheduler, sizeof(scheduler_t));
}

void run_compare_parent() {
    u64 start_pc = 0;
    int steps = 0;
    do {
        if (N64CPU.pc != start_pc) {
            printf("Running compare at 0x%08X\n", (u32)N64CPU.pc);
        }
        start_pc = N64CPU.pc;
        // Step jit
        steps = n64_system_step(true, -1);

        // Step interpreter
        send_cycles(mq_jit_to_interp_id, steps);
        // Wait for interpreter to be done
        int ran = recv_cycles(mq_interp_to_jit_id);
        if (ran != steps) {
            logfatal("interpreter ran for a different amount of time");
        }
    } while (compare());
    printf("Found a difference at pc: %016lX, ran for %d steps\n", start_pc, steps);
    printf("MIPS code:\n");
    u32 physical = resolve_virtual_address_or_die(start_pc, BUS_LOAD);
    n64_dynarec_block_t* block = &n64dynarec.blockcache[BLOCKCACHE_OUTER_INDEX(physical)][BLOCKCACHE_INNER_INDEX(physical)];
    if (physical >= N64_RDRAM_SIZE) {
        printf("outside of RDAM, can't disassemble (TODO)\n");
    } else {
        //print_multi_guest(physical, &n64sys.mem.rdram[physical], block->guest_size);
    }
    printf("IR\n");
    print_ir_block();
    printf("Host code:\n");
    print_multi_host((uintptr_t)block->run, (u8*)block->run, block->host_size);
    print_state();
}

void run_compare_child() {
    while (true) {
        int cycles = recv_cycles(mq_jit_to_interp_id);
        if (cycles < 0) {
            logfatal("Child process done.");
        }
        int taken = n64_system_step(false, cycles);
        send_cycles(mq_interp_to_jit_id, taken);
    }
}

int main(int argc, char** argv) {
    log_set_verbosity(LOG_VERBOSITY_WARN);
#ifndef INSTANT_PI_DMA
    logfatal("The emulator must be built with INSTANT_PI_DMA for this tool to be effective! (TODO: and probably other DMAs, too)");
#endif
    if (argc != 2) {
        logfatal("Usage: %s <rom>", argv[0]);
    }
    const char* rom_path = argv[1];

    key_t shmem_key = ftok(argv[0], 1);

    key_t mq_jit_to_interp_key = ftok(argv[0], 2);
    mq_jit_to_interp_id = create_and_configure_mq(mq_jit_to_interp_key);
    key_t mq_interp_to_jit_key = ftok(argv[0], 3);
    mq_interp_to_jit_id = create_and_configure_mq(mq_interp_to_jit_key);

    printf("Created and configured two queus: %08X and %08X\n", mq_jit_to_interp_id, mq_interp_to_jit_id);

    int memory_id = shmget(shmem_key, sizeof(r4300i_t), IPC_CREAT | 0777);
    r4300i_t *guest_interpreter_ptr = shmat(memory_id, NULL, 0);

    pid_t pid = fork();
    bool is_child = pid == 0;

    if (is_child) {
        n64cpu_ptr = guest_interpreter_ptr;
    }

    // TODO: enable the UI
    init_n64system(rom_path, true, false, SOFTWARE_VIDEO_TYPE, false);
    softrdp_init(&n64sys.softrdp_state, (u8 *) &n64sys.mem.rdram);
    /*
    prdp_init_internal_swapchain();
    load_imgui_ui();
    register_imgui_event_handler(imgui_handle_event);
    */
    n64_load_rom(rom_path);
    pif_rom_execute();


    u64 start_comparing_at = (s32)n64sys.mem.rom.header.program_counter;

    while (N64CPU.pc != start_comparing_at) {
        n64_system_step(false, 1);
    }

    logalways("ROM booted to %016lX, beginning comparison", start_comparing_at);

    if (is_child) {
        run_compare_child();
    } else {
        run_compare_parent();
    }
}
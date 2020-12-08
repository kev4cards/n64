#ifndef N64_R4300I_H
#define N64_R4300I_H
#include <stdbool.h>
#include <assert.h>
#include <string.h>

#include <util.h>
#include <log.h>
#include "mips_instruction_decode.h"

// Exceptions
#define EXCEPTION_INTERRUPT            0
#define EXCEPTION_COPROCESSOR_UNUSABLE 11

#define R4300I_REG_LR 31

#define R4300I_CP0_REG_INDEX    0
#define R4300I_CP0_REG_RANDOM   1
#define R4300I_CP0_REG_ENTRYLO0 2
#define R4300I_CP0_REG_ENTRYLO1 3
#define R4300I_CP0_REG_CONTEXT  4
#define R4300I_CP0_REG_PAGEMASK 5
#define R4300I_CP0_REG_WIRED    6
#define R4300I_CP0_REG_BADVADDR 8
#define R4300I_CP0_REG_COUNT    9
#define R4300I_CP0_REG_ENTRYHI  10
#define R4300I_CP0_REG_COMPARE  11
#define R4300I_CP0_REG_STATUS   12
#define R4300I_CP0_REG_CAUSE    13
#define R4300I_CP0_REG_EPC      14
#define R4300I_CP0_REG_CONFIG   16
#define R4300I_CP0_REG_WATCHLO  18
#define R4300I_CP0_REG_WATCHHI  19
#define R4300I_CP0_REG_TAGLO    28
#define R4300I_CP0_REG_TAGHI    29

#define CP0_STATUS_WRITE_MASK 0xFF57FFFF

#define OPC_CP0    0b010000
#define OPC_CP1    0b010001
#define OPC_CP2    0b010010
#define OPC_LD     0b110111
#define OPC_LUI    0b001111
#define OPC_ADDI   0b001000
#define OPC_ADDIU  0b001001
#define OPC_DADDI  0b011000
#define OPC_ANDI   0b001100
#define OPC_LBU    0b100100
#define OPC_LHU    0b100101
#define OPC_LH     0b100001
#define OPC_LW     0b100011
#define OPC_LWU    0b100111
#define OPC_BEQ    0b000100
#define OPC_BEQL   0b010100
#define OPC_BGTZ   0b000111
#define OPC_BGTZL  0b010111
#define OPC_BLEZ   0b000110
#define OPC_BLEZL  0b010110
#define OPC_BNE    0b000101
#define OPC_BNEL   0b010101
#define OPC_CACHE  0b101111
#define OPC_REGIMM 0b000001
#define OPC_SPCL   0b000000
#define OPC_SB     0b101000
#define OPC_SH     0b101001
#define OPC_SD     0b111111
#define OPC_SW     0b101011
#define OPC_ORI    0b001101
#define OPC_J      0b000010
#define OPC_JAL    0b000011
#define OPC_SLTI   0b001010
#define OPC_SLTIU  0b001011
#define OPC_XORI   0b001110
#define OPC_DADDIU 0b011001
#define OPC_LB     0b100000
#define OPC_LDC1   0b110101
#define OPC_SDC1   0b111101
#define OPC_LWC1   0b110001
#define OPC_SWC1   0b111001
#define OPC_LWL    0b100010
#define OPC_LWR    0b100110
#define OPC_SWL    0b101010
#define OPC_SWR    0b101110
#define OPC_LDL    0b011010
#define OPC_LDR    0b011011
#define OPC_SDL    0b101100
#define OPC_SDR    0b101101

// Coprocessor
#define COP_MF    0b00000
#define COP_DMF   0b00001
#define COP_CF    0b00010
#define COP_MT    0b00100
#define COP_DMT   0b00101
#define COP_CT    0b00110
#define COP_BC    0b01000


#define COP_BC_BCF  0b00000
#define COP_BC_BCT  0b00001
#define COP_BC_BCFL 0b00010
#define COP_BC_BCTL 0b00011

// Coprocessor FUNCT
#define COP_FUNCT_ADD        0b000000
#define COP_FUNCT_TLBR_SUB   0b000001
#define COP_FUNCT_TLBWI_MULT 0b000010
#define COP_FUNCT_DIV        0b000011
#define COP_FUNCT_SQRT       0b000100
#define COP_FUNCT_MOV        0b000110
#define COP_FUNCT_TLBP       0b001000
#define COP_FUNCT_TRUNC_L    0b001001
#define COP_FUNCT_TRUNC_W    0b001101
#define COP_FUNCT_ERET       0b011000
#define COP_FUNCT_CVT_S      0b100000
#define COP_FUNCT_CVT_D      0b100001
#define COP_FUNCT_CVT_W      0b100100
#define COP_FUNCT_CVT_L      0b100101
#define COP_FUNCT_NEG        0b000111
#define COP_FUNCT_C_F        0b110000
#define COP_FUNCT_C_UN       0b110001
#define COP_FUNCT_C_EQ       0b110010
#define COP_FUNCT_C_UEQ      0b110011
#define COP_FUNCT_C_OLT      0b110100
#define COP_FUNCT_C_ULT      0b110101
#define COP_FUNCT_C_OLE      0b110110
#define COP_FUNCT_C_ULE      0b110111
#define COP_FUNCT_C_SF       0b111000
#define COP_FUNCT_C_NGLE     0b111001
#define COP_FUNCT_C_SEQ      0b111010
#define COP_FUNCT_C_NGL      0b111011
#define COP_FUNCT_C_LT       0b111100
#define COP_FUNCT_C_NGE      0b111101
#define COP_FUNCT_C_LE       0b111110
#define COP_FUNCT_C_NGT      0b111111


// Floating point
#define FP_FMT_SINGLE 16
#define FP_FMT_DOUBLE 17
#define FP_FMT_W      20
#define FP_FMT_L      21

// Special
#define FUNCT_SLL    0b000000
#define FUNCT_SRL    0b000010
#define FUNCT_SRA    0b000011
#define FUNCT_SRAV   0b000111
#define FUNCT_SLLV   0b000100
#define FUNCT_SRLV   0b000110
#define FUNCT_JR     0b001000
#define FUNCT_JALR   0b001001
#define FUNCT_MFHI   0b010000
#define FUNCT_MTHI   0b010001
#define FUNCT_MFLO   0b010010
#define FUNCT_MTLO   0b010011
#define FUNCT_DSLLV  0b010100
#define FUNCT_MULT   0b011000
#define FUNCT_MULTU  0b011001
#define FUNCT_DIV    0b011010
#define FUNCT_DIVU   0b011011
#define FUNCT_DMULTU 0b011101
#define FUNCT_DDIV   0b011110
#define FUNCT_DDIVU  0b011111
#define FUNCT_ADD    0b100000
#define FUNCT_ADDU   0b100001
#define FUNCT_AND    0b100100
#define FUNCT_SUB    0b100010
#define FUNCT_SUBU   0b100011
#define FUNCT_OR     0b100101
#define FUNCT_XOR    0b100110
#define FUNCT_NOR    0b100111
#define FUNCT_SLT    0b101010
#define FUNCT_SLTU   0b101011
#define FUNCT_DADD   0b101100
#define FUNCT_DADDU  0b101101
#define FUNCT_DSUBU  0b101111
#define FUNCT_DSLL   0b111000
#define FUNCT_DSRL   0b111010
#define FUNCT_DSLL32 0b111100
#define FUNCT_DSRL32 0b111110
#define FUNCT_DSRA32 0b111111

#define FUNCT_BREAK 0b001101


// REGIMM
#define RT_BLTZ   0b00000
#define RT_BLTZL  0b00010
#define RT_BGEZ   0b00001
#define RT_BGEZL  0b00011
#define RT_BGEZAL 0b10001


typedef union cp0_status {
    word raw;
    struct {
        bool ie:1;
        bool exl:1;
        bool erl:1;
        byte ksu:2;
        bool ux:1;
        bool sx:1;
        bool kx:1;
        byte im:8;
        unsigned ds:9;
        bool re:1;
        bool fr:1;
        bool rp:1;
        bool cu0:1;
        bool cu1:1;
        bool cu2:1;
        bool cu3:1;
    };
    struct {
        unsigned:16;
        bool de:1;
        bool ce:1;
        bool ch:1;
        bool:1;
        bool sr:1;
        bool ts:1;
        bool bev:1;
        bool:1;
        bool its:1;
        unsigned:7;
    };
} cp0_status_t;

typedef union cp0_cause {
    struct {
        byte:8;
        byte interrupt_pending:8;
        unsigned:16;
    };
    struct {
        byte:2;
        byte exception_code:5;
        bool:1;
        bool ip0:1;
        bool ip1:1;
        bool ip2:1;
        bool ip3:1;
        bool ip4:1;
        bool ip5:1;
        bool ip6:1;
        bool ip7:1;
        unsigned:12;
        byte coprocessor_error:2;
        bool:1;
        bool branch_delay:1;
    };
    word raw;
} cp0_cause_t;

typedef union cp0_entry_lo {
    word raw;
    struct {
        bool g:1;
        bool v:1;
        bool d:1;
        unsigned c:3;
        unsigned pfn:20;
        unsigned:6;
    };
} cp0_entry_lo_t;

typedef union cp0_page_mask {
    word raw;
    struct {
        unsigned:13;
        unsigned mask:12;
        unsigned:7;
    };
} cp0_page_mask_t;

typedef union cp0_entry_hi {
    struct {
        unsigned asid:8;
        unsigned:5;
        unsigned vpn2:19;
    };
    word raw;
} cp0_entry_hi_t;

typedef struct tlb_entry {
    union {
        struct {
            bool global:1;
            bool valid:1;
            bool dirty:1;
            byte c:3;
            unsigned entry:24;
            unsigned:2;
        };
        word raw;
    } entry_lo0;

    union {
        struct {
            bool global:1;
            bool valid:1;
            bool dirty:1;
            byte c:3;
            unsigned entry:24;
            unsigned:2;
        };
        word raw;
    } entry_lo1;

    union {
        word raw;
        struct {
            unsigned asid:8;
            unsigned:4;
            bool g:1;
            unsigned vpn2:19;
        };
    } entry_hi;

    union {
        struct {
            unsigned:13;
            unsigned mask:12;
            unsigned:7;
        };
        word raw;
    } page_mask;

    // "parsed"
    bool global;
    bool valid;
    byte asid;

} tlb_entry_t;

typedef union watch_lo {
    word raw;
    struct {
        bool w:1;
        bool r:1;
        bool:1;
        unsigned paddr0:29;
    };
} watch_lo_t;

static_assert(sizeof(watch_lo_t) == 4, "watch_lo_t wrong size!");

typedef struct cp0 {
    word index;
    word random;
    cp0_entry_lo_t entry_lo0;
    cp0_entry_lo_t entry_lo1;
    word context;
    cp0_page_mask_t page_mask;
    word wired;
    word r7;
    word bad_vaddr;
    dword count;
    cp0_entry_hi_t entry_hi;
    word compare;
    cp0_status_t status;
    cp0_cause_t cause;
    word EPC;
    word PRId;
    word config;
    word lladdr;
    watch_lo_t watch_lo;
    word watch_hi;
    word x_context;
    word r21;
    word r22;
    word r23;
    word r24;
    word r25;
    word parity_error;
    word cache_error;
    word tag_lo;
    word tag_hi;
    word error_epc;
    word r31;

    tlb_entry_t tlb[32];
} cp0_t;

typedef union fcr0 {
    word raw;
} fcr0_t;

typedef union fcr31 {
    word raw;

    struct {
        byte rounding_mode:2;
        bool flag_inexact_operation:1;
        bool flag_underflow:1;
        bool flag_overflow:1;
        bool flag_division_by_zero:1;
        bool flag_invalid_operation:1;
        bool enable_inexact_operation:1;
        bool enable_underflow:1;
        bool enable_overflow:1;
        bool enable_division_by_zero:1;
        bool enable_invalid_operation:1;
        bool cause_inexact_operation:1;
        bool cause_underflow:1;
        bool cause_overflow:1;
        bool cause_division_by_zero:1;
        bool cause_invalid_operation:1;
        bool cause_unimplemented_operation:1;
        unsigned:5;
        bool compare:1;
        bool fs:1;
        unsigned:7;
    };
} fcr31_t;

typedef union fgr {
    dword raw;
    struct {
        word lo:32;
        word hi:32;
    } __attribute__((packed));
} fgr_t;

static_assert(sizeof(fgr_t) == sizeof(dword), "fgr_t must be 64 bits");

typedef struct r4300i {
    dword gpr[32];

    word pc;
    word next_pc;
    word prev_pc;

    dword mult_hi;
    dword mult_lo;

    fcr0_t  fcr0;
    fcr31_t fcr31;

    fgr_t f[32];

    cp0_t cp0;

    // Cached value of `cp0.cause.interrupt_pending & cp0.status.im`
    byte interrupts;

    // In a branch delay slot?
    bool branch;

    // Did an exception just happen?
    bool exception;

    byte (*read_byte)(word);
    void (*write_byte)(word, byte);

    half (*read_half)(word);
    void (*write_half)(word, half);

    word (*read_word)(word);
    void (*write_word)(word, word);

    dword (*read_dword)(word);
    void (*write_dword)(word, dword);
} r4300i_t;

typedef void(*mipsinstr_handler_t)(r4300i_t*, mips_instruction_t);

void r4300i_step(r4300i_t* cpu);
void r4300i_handle_exception(r4300i_t* cpu, word pc, word code, word coprocessor_error);
mipsinstr_handler_t r4300i_instruction_decode(word pc, mips_instruction_t instr);
void r4300i_interrupt_update(r4300i_t* cpu);

extern const char* register_names[];
extern const char* cp0_register_names[];

INLINE void set_register(r4300i_t* cpu, byte r, dword value) {
    logtrace("Setting $%s (r%d) to [0x%016lX]", register_names[r], r, value);
    if (r != 0) {
        cpu->gpr[r] = value;
    }
}

INLINE dword get_register(r4300i_t* cpu, byte r) {
    dword value = cpu->gpr[r];
    logtrace("Reading $%s (r%d): 0x%08lX", register_names[r], r, value);
    return value;
}

INLINE void on_change_fr(r4300i_t* cpu, cp0_status_t oldstatus) {
    //logfatal("FR changed from %d to %d!", oldstatus.fr, cpu->cp0.status.fr);
}

INLINE void set_cp0_register(r4300i_t* cpu, byte r, word value) {
    switch (r) {
        case R4300I_CP0_REG_INDEX:
            cpu->cp0.index = value;
            break;
        case R4300I_CP0_REG_RANDOM:
            break;
        case R4300I_CP0_REG_COUNT:
            cpu->cp0.count = value;
            break;
        case R4300I_CP0_REG_CAUSE: {
            cp0_cause_t newcause;
            newcause.raw = value;
            cpu->cp0.cause.ip0 = newcause.ip0;
            cpu->cp0.cause.ip1 = newcause.ip1;
            break;
        }
        case R4300I_CP0_REG_TAGLO: // Used for the cache, which is unimplemented.
            cpu->cp0.tag_lo = value;
            break;
        case R4300I_CP0_REG_TAGHI: // Used for the cache, which is unimplemented.
            cpu->cp0.tag_hi = value;
            break;
        case R4300I_CP0_REG_COMPARE:
            loginfo("$Compare written with 0x%08X (count is now 0x%08lX)", value, cpu->cp0.count);
            cpu->cp0.cause.ip7 = false;
            cpu->cp0.compare = value;
            break;
        case R4300I_CP0_REG_STATUS: {
            cp0_status_t oldstatus = cpu->cp0.status;

            cpu->cp0.status.raw &= value & ~CP0_STATUS_WRITE_MASK;
            cpu->cp0.status.raw |= value & CP0_STATUS_WRITE_MASK;

            if (oldstatus.fr != cpu->cp0.status.fr) {
                on_change_fr(cpu, oldstatus);
            }

            loginfo("    CP0 status: ie:  %d", cpu->cp0.status.ie);
            loginfo("    CP0 status: exl: %d", cpu->cp0.status.exl);
            loginfo("    CP0 status: erl: %d", cpu->cp0.status.erl);
            loginfo("    CP0 status: ksu: %d", cpu->cp0.status.ksu);
            loginfo("    CP0 status: ux:  %d", cpu->cp0.status.ux);
            loginfo("    CP0 status: sx:  %d", cpu->cp0.status.sx);
            loginfo("    CP0 status: kx:  %d", cpu->cp0.status.kx);
            loginfo("    CP0 status: im:  %d", cpu->cp0.status.im);
            loginfo("    CP0 status: ds:  %d", cpu->cp0.status.ds);
            loginfo("    CP0 status: re:  %d", cpu->cp0.status.re);
            loginfo("    CP0 status: fr:  %d", cpu->cp0.status.fr);
            loginfo("    CP0 status: rp:  %d", cpu->cp0.status.rp);
            loginfo("    CP0 status: cu0: %d", cpu->cp0.status.cu0);
            loginfo("    CP0 status: cu1: %d", cpu->cp0.status.cu1);
            loginfo("    CP0 status: cu2: %d", cpu->cp0.status.cu2);
            loginfo("    CP0 status: cu3: %d", cpu->cp0.status.cu3);

            r4300i_interrupt_update(cpu);
            break;
        }
        case R4300I_CP0_REG_ENTRYLO0:
            cpu->cp0.entry_lo0.raw = value;
            break;
        case R4300I_CP0_REG_ENTRYLO1:
            cpu->cp0.entry_lo1.raw = value;
            break;
        case 7:
            logfatal("CP0 Reg 7 write?");
            break;
        case R4300I_CP0_REG_ENTRYHI:
            cpu->cp0.entry_hi.raw = value;
            break;
        case R4300I_CP0_REG_PAGEMASK:
            cpu->cp0.page_mask.raw = value;
            break;
        case R4300I_CP0_REG_EPC:
            cpu->cp0.EPC = value;
            break;
        case R4300I_CP0_REG_CONFIG:
            cpu->cp0.config = value;
            break;
        case R4300I_CP0_REG_WATCHLO:
            cpu->cp0.watch_lo.raw = value;
            unimplemented(cpu->cp0.watch_lo.r, "Read exception enabled in CP0 watch_lo!");
            unimplemented(cpu->cp0.watch_lo.w, "Write exception enabled in CP0 watch_lo!");
            break;
        case R4300I_CP0_REG_WATCHHI:
            cpu->cp0.watch_hi = value;
            break;
        default:
            logfatal("Unsupported CP0 $%s (%d) set: 0x%08X", cp0_register_names[r], r, value);
    }

    loginfo("CP0 $%s = 0x%08X", cp0_register_names[r], value);
}

INLINE word get_cp0_register(r4300i_t* cpu, byte r) {
    switch (r) {
        case R4300I_CP0_REG_ENTRYLO0:
            return cpu->cp0.entry_lo0.raw;
        case R4300I_CP0_REG_BADVADDR:
            return cpu->cp0.bad_vaddr;
        case R4300I_CP0_REG_STATUS:
            return cpu->cp0.status.raw;
        case R4300I_CP0_REG_ENTRYHI:
            return cpu->cp0.entry_hi.raw;
        case R4300I_CP0_REG_CAUSE:
            return cpu->cp0.cause.raw;
        case R4300I_CP0_REG_EPC:
            return cpu->cp0.EPC;
        case R4300I_CP0_REG_COUNT: {
            dword shifted = cpu->cp0.count >> 1;
            return (word)shifted;
        }
        case R4300I_CP0_REG_COMPARE:
            return cpu->cp0.compare;
        case R4300I_CP0_REG_INDEX:
            return cpu->cp0.index & 0x8000003F;
        case R4300I_CP0_REG_CONTEXT:
            return cpu->cp0.context;
        case R4300I_CP0_REG_PAGEMASK:
            return cpu->cp0.page_mask.raw;
        case R4300I_CP0_REG_WIRED:
            return cpu->cp0.wired;
        default:
            logfatal("Unsupported CP0 $%s (%d) read", cp0_register_names[r], r);
    }
}

INLINE void set_fpu_register_dword(r4300i_t* cpu, byte r, dword value) {
    cpu->f[r].raw = value;
}

INLINE dword get_fpu_register_dword(r4300i_t* cpu, byte r) {
    return cpu->f[r].raw;
}

INLINE void set_fpu_register_word(r4300i_t* cpu, byte r, word value) {
    if (!cpu->cp0.status.fr) {
        if ((r & 1) == 0) {
            cpu->f[r].lo = value;
        } else {
            cpu->f[r - 1].hi = value;
        }
    } else {
        logfatal("Unimplemented!");
    }
}

INLINE word get_fpu_register_word(r4300i_t* cpu, byte r) {
    if (!cpu->cp0.status.fr) {
        if ((r & 1) == 0) {
            return cpu->f[r].lo;
        } else {
            return cpu->f[r - 1].hi;
        }
    } else {
        logfatal("Unimplemented!");
    }
}

INLINE void set_fpu_register_double(r4300i_t* cpu, byte r, double value) {
    static_assert(sizeof(double) == sizeof(dword), "double and dword need to both be 64 bits for this to work.");

    dword rawvalue;
    memcpy(&rawvalue, &value, sizeof(double));
    set_fpu_register_dword(cpu, r, rawvalue);
}

INLINE double get_fpu_register_double(r4300i_t* cpu, byte r) {
    static_assert(sizeof(double) == sizeof(dword), "double and dword need to both be 64 bits for this to work.");
    double doublevalue;
    dword rawvalue = get_fpu_register_dword(cpu, r);
    memcpy(&doublevalue, &rawvalue, sizeof(double));
    return doublevalue;
}

INLINE void set_fpu_register_float(r4300i_t* cpu, byte r, float value) {
    static_assert(sizeof(float) == sizeof(word), "float and word need to both be 32 bits for this to work.");

    word rawvalue;
    memcpy(&rawvalue, &value, sizeof(float));
    set_fpu_register_word(cpu, r, rawvalue);
}

INLINE float get_fpu_register_float(r4300i_t* cpu, byte r) {
    static_assert(sizeof(float) == sizeof(word), "float and word need to both be 32 bits for this to work.");
    word rawvalue = get_fpu_register_word(cpu, r);
    float floatvalue;
    memcpy(&floatvalue, &rawvalue, sizeof(float));
    return floatvalue;
}

#endif //N64_R4300I_H

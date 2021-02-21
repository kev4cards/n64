#include <stdio.h>
#include <log.h>
#include <string.h>
#include "softrdp.h"

typedef uint8_t byte;
typedef uint16_t half;
typedef uint32_t word;
typedef uint64_t dword;

typedef int8_t sbyte;
typedef int16_t shalf;
typedef int32_t sword;
typedef int64_t sdword;

void init_softrdp(softrdp_state_t* state, byte* rdramptr) {
    state->rdram = rdramptr;
}

typedef enum rdp_command {
    RDP_COMMAND_FILL_TRIANGLE = 0x08,
    RDP_COMMAND_FILL_ZBUFFER_TRIANGLE = 0x09,
    RDP_COMMAND_TEXTURE_TRIANGLE = 0x0a,
    RDP_COMMAND_TEXTURE_ZBUFFER_TRIANGLE = 0x0b,
    RDP_COMMAND_SHADE_TRIANGLE = 0x0c,
    RDP_COMMAND_SHADE_ZBUFFER_TRIANGLE = 0x0d,
    RDP_COMMAND_SHADE_TEXTURE_TRIANGLE = 0x0e,
    RDP_COMMAND_SHADE_TEXTURE_ZBUFFER_TRIANGLE = 0x0f,
    RDP_COMMAND_TEXTURE_RECTANGLE = 0x24,
    RDP_COMMAND_TEXTURE_RECTANGLE_FLIP = 0x25,
    RDP_COMMAND_SYNC_LOAD = 0x26,
    RDP_COMMAND_SYNC_PIPE = 0x27,
    RDP_COMMAND_SYNC_TILE = 0x28,
    RDP_COMMAND_SYNC_FULL = 0x29,
    RDP_COMMAND_SET_KEY_GB = 0x2a,
    RDP_COMMAND_SET_KEY_R = 0x2b,
    RDP_COMMAND_SET_CONVERT = 0x2c,
    RDP_COMMAND_SET_SCISSOR = 0x2d,
    RDP_COMMAND_SET_PRIM_DEPTH = 0x2e,
    RDP_COMMAND_SET_OTHER_MODES = 0x2f,
    RDP_COMMAND_LOAD_TLUT = 0x30,
    RDP_COMMAND_SET_TILE_SIZE = 0x32,
    RDP_COMMAND_LOAD_BLOCK = 0x33,
    RDP_COMMAND_LOAD_TILE = 0x34,
    RDP_COMMAND_SET_TILE = 0x35,
    RDP_COMMAND_FILL_RECTANGLE = 0x36,
    RDP_COMMAND_SET_FILL_COLOR = 0x37,
    RDP_COMMAND_SET_FOG_COLOR = 0x38,
    RDP_COMMAND_SET_BLEND_COLOR = 0x39,
    RDP_COMMAND_SET_PRIM_COLOR = 0x3a,
    RDP_COMMAND_SET_ENV_COLOR = 0x3b,
    RDP_COMMAND_SET_COMBINE = 0x3c,
    RDP_COMMAND_SET_TEXTURE_IMAGE = 0x3d,
    RDP_COMMAND_SET_MASK_IMAGE = 0x3e,
    RDP_COMMAND_SET_COLOR_IMAGE = 0x3f
} rdp_command_t;

#ifndef INLINE
#define INLINE static inline __attribute__((always_inline))
#endif

#define DEF_RDP_COMMAND(name) INLINE void rdp_command_##name(softrdp_state_t* rdp, int command_length, const word* buffer)
#define EXEC_RDP_COMMAND(name) rdp_command_##name(rdp, command_length, buffer); break
#define MASK_LEN(n) ((1 << (n)) - 1)
#define BITS(hi, lo) ((buffer[((command_length) - 1) - ((lo) / 32)] >> ((lo) & 0b11111)) & MASK_LEN((hi) - (lo) + 1))
#define BIT(index) (((buffer[((command_length) - 1) - ((index) / 32)] >> ((index) & 0b11111)) & 1) != 0)

INLINE void rdram_write32(softrdp_state_t* rdp, word address, word value) {
    memcpy(&rdp->rdram[address], &value, sizeof(word));
}

INLINE bool check_scissor(softrdp_state_t* rdp, int x, int y) {
    //return (x >= rdp->scissor.xl && x <= rdp->scissor.xh && y >= rdp->scissor.yl && y <= rdp->scissor.yh);
    return true;
}


DEF_RDP_COMMAND(fill_triangle) {
    logfatal("rdp_fill_triangle unimplemented");
}

DEF_RDP_COMMAND(fill_zbuffer_triangle) {
    logfatal("rdp_fill_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(texture_triangle) {
    logfatal("rdp_texture_triangle unimplemented");
}

DEF_RDP_COMMAND(texture_zbuffer_triangle) {
    logfatal("rdp_texture_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_triangle) {
    logfatal("rdp_shade_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_zbuffer_triangle) {
    logfatal("rdp_shade_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_texture_triangle) {
    logfatal("rdp_shade_texture_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_texture_zbuffer_triangle) {
    logfatal("rdp_shade_texture_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(texture_rectangle) {
    logfatal("rdp_texture_rectangle unimplemented");
}

DEF_RDP_COMMAND(texture_rectangle_flip) {
    logfatal("rdp_texture_rectangle_flip unimplemented");
}

DEF_RDP_COMMAND(sync_load) {
    logfatal("rdp_sync_load unimplemented");
}

DEF_RDP_COMMAND(sync_pipe) {
    logwarn("rdp_sync_pipe unimplemented");
}

DEF_RDP_COMMAND(sync_tile) {
    logfatal("rdp_sync_tile unimplemented");
}

DEF_RDP_COMMAND(sync_full) {
    logwarn("rdp_sync_full unimplemented");
}

DEF_RDP_COMMAND(set_key_gb) {
    logfatal("rdp_set_key_gb unimplemented");
}

DEF_RDP_COMMAND(set_key_r) {
    logfatal("rdp_set_key_r unimplemented");
}

DEF_RDP_COMMAND(set_convert) {
    logfatal("rdp_set_convert unimplemented");
}

DEF_RDP_COMMAND(set_scissor) {
    rdp->scissor.yl = BITS(11, 0);
    rdp->scissor.xl = BITS(23, 12);

    rdp->scissor.yh = BITS(43, 32);
    rdp->scissor.xh = BITS(55, 44);

    rdp->scissor.f = BIT(25);
    rdp->scissor.o = BIT(24);
}

DEF_RDP_COMMAND(set_prim_depth) {
    rdp->primitive_z       = BITS(31, 16);
    rdp->primitive_delta_z = BITS(15, 0);
}

DEF_RDP_COMMAND(set_other_modes) {
    logwarn("rdp_set_other_modes unimplemented");
}

DEF_RDP_COMMAND(load_tlut) {
    logfatal("rdp_load_tlut unimplemented");
}

DEF_RDP_COMMAND(set_tile_size) {
    logfatal("rdp_set_tile_size unimplemented");
}

DEF_RDP_COMMAND(load_block) {
    logfatal("rdp_load_block unimplemented");
}

DEF_RDP_COMMAND(load_tile) {
    logfatal("rdp_load_tile unimplemented");
}

DEF_RDP_COMMAND(set_tile) {
    logfatal("rdp_set_tile unimplemented");
}

DEF_RDP_COMMAND(fill_rectangle) {
    int xl = BITS(55, 44);
    int yl = BITS(43, 32);

    int xh = BITS(23, 12);
    int yh = BITS(11, 0);

    logalways("Fill rectangle (%d, %d) (%d, %d)", xh, yh, xl, yl);

    unimplemented(rdp->color_image.format != 0, "Fill rect when color image format not RGBA");
    unimplemented(rdp->color_image.size != 3, "Fill rect when color image size not 32bpp");

    int bytes_per_pixel = 4;

    int y_range = (yl - yh) / bytes_per_pixel + 1;
    int x_range = (xl - xh) / bytes_per_pixel + 1;

    logalways("y range: %d x range: %d", y_range, x_range);

    for (int y = 0; y < y_range; y++) {
        int y_pixel = y + yh / 4;
        int yofs = (y_pixel * bytes_per_pixel * rdp->color_image.width);
        for (int x = 0; x < x_range; x++) {
            int x_pixel = x + xh / 4;
            int xofs = xh + (x * bytes_per_pixel);
            word addr = rdp->color_image.dram_addr + yofs + xofs;
            if (check_scissor(rdp, x_pixel, y_pixel)) {
                rdram_write32(rdp, addr, rdp->fill_color);
            }
        }
    }

}

DEF_RDP_COMMAND(set_fill_color) {
    rdp->fill_color = buffer[1];
    logalways("Fill color: 0x%08X", buffer[1]);
}

DEF_RDP_COMMAND(set_fog_color) {
    logfatal("rdp_set_fog_color unimplemented");
}

DEF_RDP_COMMAND(set_blend_color) {
    logfatal("rdp_set_blend_color unimplemented");
}

DEF_RDP_COMMAND(set_prim_color) {
    logfatal("rdp_set_prim_color unimplemented");
}

DEF_RDP_COMMAND(set_env_color) {
    logfatal("rdp_set_env_color unimplemented");
}

DEF_RDP_COMMAND(set_combine) {
    logfatal("rdp_set_combine unimplemented");
}

DEF_RDP_COMMAND(set_texture_image) {
    logfatal("rdp_set_texture_image unimplemented");
}

DEF_RDP_COMMAND(set_mask_image) {
    logfatal("rdp_set_mask_image unimplemented");
}

DEF_RDP_COMMAND(set_color_image) {
    logalways("Set color image:");
    rdp->color_image.format    = BITS(55, 53);
    rdp->color_image.size      = BITS(52, 51);
    rdp->color_image.width     = BITS(41, 32) + 1;
    rdp->color_image.dram_addr = BITS(25, 0);
    logalways("Format: %d", rdp->color_image.format);
    logalways("Size: %d",   rdp->color_image.size);
    logalways("Width: %d",  rdp->color_image.width);
    logalways("DRAM addr: 0x%08X", rdp->color_image.dram_addr);
}

void enqueue_command_softrdp(softrdp_state_t* rdp, int command_length, word* buffer) {
    rdp_command_t command = (buffer[0] >> 24) & 0x3F;
    printf("command: 0x");
    for (int i = 0; i < command_length; i++) {
        printf("%08X", buffer[i]);
    }
    printf("\n");

    switch (command) {
        case RDP_COMMAND_FILL_TRIANGLE:                  EXEC_RDP_COMMAND(fill_triangle);
        case RDP_COMMAND_FILL_ZBUFFER_TRIANGLE:          EXEC_RDP_COMMAND(fill_zbuffer_triangle);
        case RDP_COMMAND_TEXTURE_TRIANGLE:               EXEC_RDP_COMMAND(texture_triangle);
        case RDP_COMMAND_TEXTURE_ZBUFFER_TRIANGLE:       EXEC_RDP_COMMAND(texture_zbuffer_triangle);
        case RDP_COMMAND_SHADE_TRIANGLE:                 EXEC_RDP_COMMAND(shade_triangle);
        case RDP_COMMAND_SHADE_ZBUFFER_TRIANGLE:         EXEC_RDP_COMMAND(shade_zbuffer_triangle);
        case RDP_COMMAND_SHADE_TEXTURE_TRIANGLE:         EXEC_RDP_COMMAND(shade_texture_triangle);
        case RDP_COMMAND_SHADE_TEXTURE_ZBUFFER_TRIANGLE: EXEC_RDP_COMMAND(shade_texture_zbuffer_triangle);
        case RDP_COMMAND_TEXTURE_RECTANGLE:              EXEC_RDP_COMMAND(texture_rectangle);
        case RDP_COMMAND_TEXTURE_RECTANGLE_FLIP:         EXEC_RDP_COMMAND(texture_rectangle_flip);
        case RDP_COMMAND_SYNC_LOAD:                      EXEC_RDP_COMMAND(sync_load);
        case RDP_COMMAND_SYNC_PIPE:                      EXEC_RDP_COMMAND(sync_pipe);
        case RDP_COMMAND_SYNC_TILE:                      EXEC_RDP_COMMAND(sync_tile);
        case RDP_COMMAND_SYNC_FULL:                      EXEC_RDP_COMMAND(sync_full);
        case RDP_COMMAND_SET_KEY_GB:                     EXEC_RDP_COMMAND(set_key_gb);
        case RDP_COMMAND_SET_KEY_R:                      EXEC_RDP_COMMAND(set_key_r);
        case RDP_COMMAND_SET_CONVERT:                    EXEC_RDP_COMMAND(set_convert);
        case RDP_COMMAND_SET_SCISSOR:                    EXEC_RDP_COMMAND(set_scissor);
        case RDP_COMMAND_SET_PRIM_DEPTH:                 EXEC_RDP_COMMAND(set_prim_depth);
        case RDP_COMMAND_SET_OTHER_MODES:                EXEC_RDP_COMMAND(set_other_modes);
        case RDP_COMMAND_LOAD_TLUT:                      EXEC_RDP_COMMAND(load_tlut);
        case RDP_COMMAND_SET_TILE_SIZE:                  EXEC_RDP_COMMAND(set_tile_size);
        case RDP_COMMAND_LOAD_BLOCK:                     EXEC_RDP_COMMAND(load_block);
        case RDP_COMMAND_LOAD_TILE:                      EXEC_RDP_COMMAND(load_tile);
        case RDP_COMMAND_SET_TILE:                       EXEC_RDP_COMMAND(set_tile);
        case RDP_COMMAND_FILL_RECTANGLE:                 EXEC_RDP_COMMAND(fill_rectangle);
        case RDP_COMMAND_SET_FILL_COLOR:                 EXEC_RDP_COMMAND(set_fill_color);
        case RDP_COMMAND_SET_FOG_COLOR:                  EXEC_RDP_COMMAND(set_fog_color);
        case RDP_COMMAND_SET_BLEND_COLOR:                EXEC_RDP_COMMAND(set_blend_color);
        case RDP_COMMAND_SET_PRIM_COLOR:                 EXEC_RDP_COMMAND(set_prim_color);
        case RDP_COMMAND_SET_ENV_COLOR:                  EXEC_RDP_COMMAND(set_env_color);
        case RDP_COMMAND_SET_COMBINE:                    EXEC_RDP_COMMAND(set_combine);
        case RDP_COMMAND_SET_TEXTURE_IMAGE:              EXEC_RDP_COMMAND(set_texture_image);
        case RDP_COMMAND_SET_MASK_IMAGE:                 EXEC_RDP_COMMAND(set_mask_image);
        case RDP_COMMAND_SET_COLOR_IMAGE:                EXEC_RDP_COMMAND(set_color_image);

        default: logfatal("Unknown RDP command: %02X", command);
    }
}

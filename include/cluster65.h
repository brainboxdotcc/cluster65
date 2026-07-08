#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <immintrin.h>
#include <memory>

constexpr int reg_a = 0;
constexpr int reg_x = 8;
constexpr int reg_y = 16;
constexpr int reg_s = 24;
constexpr int reg_flags = 32;
constexpr int reg_pc = 40;

constexpr uint64_t mask8 = 0xff;
constexpr uint64_t mask16 = 0xffff;

constexpr uint64_t pc_mask = mask16 << reg_pc;
constexpr uint64_t a_mask = mask8 << reg_a;
constexpr uint64_t x_mask = mask8 << reg_x;
constexpr uint64_t y_mask = mask8 << reg_y;
constexpr uint64_t s_mask = mask8 << reg_s;
constexpr uint64_t flags_mask = mask8 << reg_flags;

constexpr uint8_t flag_c = 1 << 0;
constexpr uint8_t flag_z = 1 << 1;
constexpr uint8_t flag_i = 1 << 2;
constexpr uint8_t flag_d = 1 << 3;
constexpr uint8_t flag_b = 1 << 4;
constexpr uint8_t flag_u = 1 << 5;
constexpr uint8_t flag_v = 1 << 6;
constexpr uint8_t flag_n = 1 << 7;

struct cpu_group
{
    alignas(32) uint64_t cpu[4];

    /*
     * Interpolated RAM.
     *
     * Byte for CPU n at address addr lives at:
     *
     *     ram[(addr * 4) + n]
     *
     * Padding avoids AVX2 64-bit gather reading past the end
     * when fetching near &ffff.
     */
    alignas(32) uint8_t ram[(65536 * 4) + 8];
};

using opcode_handler = void (*)(cpu_group&, int);

inline uint8_t& mem(cpu_group& group, uint16_t addr, int lane)
{
    return group.ram[(addr * 4) + lane];
}

inline uint16_t get_pc(uint64_t cpu)
{
    return (cpu >> reg_pc) & mask16;
}

inline uint64_t set_pc(uint64_t cpu, uint16_t pc)
{
    cpu &= ~pc_mask;
    cpu |= uint64_t(pc) << reg_pc;
    return cpu;
}

inline __m256i lane_ids()
{
    return _mm256_set_epi64x(3, 2, 1, 0);
}

inline __m256i mask_from_bits(int active)
{
    return _mm256_set_epi64x(
        (active & 8) ? -1LL : 0,
        (active & 4) ? -1LL : 0,
        (active & 2) ? -1LL : 0,
        (active & 1) ? -1LL : 0
    );
}

inline __m256i fetch8(cpu_group& group, int active)
{
    __m256i cpu = _mm256_load_si256((__m256i*)group.cpu);

    // Extract PC from each packed CPU state.
    __m256i pc = _mm256_and_si256(
        _mm256_srli_epi64(cpu, reg_pc),
        _mm256_set1_epi64x(mask16)
    );

    // RAM offset = pc * 4 + lane.
    __m256i offsets = _mm256_add_epi64(
        _mm256_slli_epi64(pc, 2),
        lane_ids()
    );

    // AVX2 gathers 64-bit chunks, so keep only the low byte.
    __m256i values = _mm256_i64gather_epi64(
        (const long long*)group.ram,
        offsets,
        1
    );

    values = _mm256_and_si256(values, _mm256_set1_epi64x(mask8));

    // Increment PC only for active lanes.
    __m256i next_pc = _mm256_and_si256(
        _mm256_add_epi64(pc, _mm256_set1_epi64x(1)),
        _mm256_set1_epi64x(mask16)
    );

    __m256i next_cpu = _mm256_or_si256(
        _mm256_andnot_si256(_mm256_set1_epi64x(pc_mask), cpu),
        _mm256_slli_epi64(next_pc, reg_pc)
    );

    __m256i mask = mask_from_bits(active);

    cpu = _mm256_or_si256(
        _mm256_and_si256(mask, next_cpu),
        _mm256_andnot_si256(mask, cpu)
    );

    _mm256_store_si256((__m256i*)group.cpu, cpu);

    return values;
}

inline __m256i fetch16(cpu_group& group, int active)
{
    __m256i lo = fetch8(group, active);
    __m256i hi = fetch8(group, active);

    return _mm256_or_si256(lo, _mm256_slli_epi64(hi, 8));
}

inline int lanes_equal(__m256i value, uint8_t opcode)
{
    __m256i cmp = _mm256_cmpeq_epi64(value, _mm256_set1_epi64x(opcode));

    return _mm256_movemask_pd(_mm256_castsi256_pd(cmp));
}

inline __m256i replace_field(__m256i cpu, __m256i value, uint64_t mask, int shift)
{
    return _mm256_or_si256(
        _mm256_andnot_si256(_mm256_set1_epi64x(mask), cpu),
        _mm256_slli_epi64(value, shift)
    );
}

inline __m256i field8(__m256i cpu, int shift)
{
    return _mm256_and_si256(
        _mm256_srli_epi64(cpu, shift),
        _mm256_set1_epi64x(mask8)
    );
}

inline __m256i set_zn(__m256i cpu, __m256i value)
{
    __m256i flags = field8(cpu, reg_flags);

    flags = _mm256_andnot_si256(
        _mm256_set1_epi64x(flag_z | flag_n),
        flags
    );

    __m256i z = _mm256_cmpeq_epi64(value, _mm256_setzero_si256());

    __m256i n = _mm256_and_si256(
        value,
        _mm256_set1_epi64x(0x80)
    );

    flags = _mm256_or_si256(
        flags,
        _mm256_and_si256(z, _mm256_set1_epi64x(flag_z))
    );

    flags = _mm256_or_si256(flags, n);

    return replace_field(cpu, flags, flags_mask, reg_flags);
}

inline void store_active_cpu(cpu_group& group, __m256i cpu, __m256i next_cpu, int active)
{
    __m256i mask = mask_from_bits(active);

    cpu = _mm256_or_si256(
        _mm256_and_si256(mask, next_cpu),
        _mm256_andnot_si256(mask, cpu)
    );

    _mm256_store_si256((__m256i*)group.cpu, cpu);
}

inline void write8(cpu_group& group, __m256i addr, __m256i value, int active)
{
    alignas(32) uint64_t a[4] = {};
    alignas(32) uint64_t v[4] = {};

    _mm256_store_si256((__m256i*)a, addr);
    _mm256_store_si256((__m256i*)v, value);

    for (int lane = 0; lane < 4; lane++) {
        if (active & (1 << lane)) {
            mem(group, a[lane] & 0xffff, lane) = v[lane] & 0xff;
        }
    }
}

inline void branch_relative(cpu_group& group, int active, int taken)
{
    __m256i offset = fetch8(group, active);

    alignas(32) uint64_t o[4];
    alignas(32) uint64_t c[4];

    _mm256_store_si256((__m256i*)o, offset);
    _mm256_store_si256((__m256i*)c, _mm256_load_si256((__m256i*)group.cpu));

    for (int lane = 0; lane < 4; lane++) {
        if (!(taken & (1 << lane))) {
            continue;
        }

        int8_t rel = int8_t(o[lane] & 0xff);
        uint16_t pc = get_pc(c[lane]);
        c[lane] = set_pc(c[lane], pc + rel);
    }

    _mm256_store_si256((__m256i*)group.cpu, _mm256_load_si256((__m256i*)c));
}

inline uint8_t get_flags(uint64_t cpu)
{
    return (cpu >> reg_flags) & mask8;
}
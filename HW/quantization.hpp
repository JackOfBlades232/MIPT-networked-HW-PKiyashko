#pragma once
#include "math_utils.hpp"
#include <cassert>
#include <limits>

// @TODO: assert that sizeof(T) == sizeof(Packed...<T, ...>) for every type

#define CHECK_NUM_BITS(_type, _num_bits) static_assert(((_type)1 << (_num_bits-1)) < std::numeric_limits<_type>::max())

template <class T>
constexpr T first_bits_mask(int num_bits)
{
    return (1 << num_bits) - 1;
}

template <class T>
T pack_float(float val, float lo, float hi, int num_bits)
{
    float scaled = ((clamp(val, lo, hi) - lo) / (hi - lo)) * (float)((1 << num_bits) - 1);
    return (T)scaled;
}

template <class T>
float unpack_float(T val, float lo, float hi, int num_bits)
{
    if (lo <= 0.f && hi >= 0.f && pack_float<T>(0.f, lo, hi, num_bits) == val)
        return 0.f;

    return ((float)val / ((1 << num_bits) - 1)) * (hi - lo) + lo;
}

template <class T, int t_num_bits>
class PackedFloat {
    CHECK_NUM_BITS(T, t_num_bits);

    T m_packed_val;

public:
    PackedFloat(float val, float lo, float hi) { Pack(val, lo, hi); }
    PackedFloat(T packed_val) : m_packed_val(packed_val) {}
    PackedFloat() = default;

    void Pack(float val, float lo, float hi) { m_packed_val = pack_float<T>(val, lo, hi, t_num_bits); }
    float Unpack(float lo, float hi) { return unpack_float(m_packed_val, lo, hi, t_num_bits); }
};

template <class T, int t_x_num_bits, int t_y_num_bits>
class PackedFloat2 {
    CHECK_NUM_BITS(T, t_x_num_bits + t_y_num_bits);

    T m_packed_val;

public:
    PackedFloat2(float2 val, float2 x_lohi, float2 y_lohi) { Pack(val, x_lohi, y_lohi); }
    PackedFloat2(float2 val, float lo, float hi)           { Pack(val, lo, hi); }
    PackedFloat2(T packed_val) : m_packed_val(packed_val) {}
    PackedFloat2() = default;

    void Pack(float2 val, float2 x_lohi, float2 y_lohi) {
        m_packed_val =
            (pack_float<T>(val.x, x_lohi.x, x_lohi.y, t_x_num_bits) << t_y_num_bits) |
            pack_float<T>(val.y, y_lohi.x, y_lohi.y, t_y_num_bits);
    }
    void Pack(float2 val, float lo, float hi) { Pack(val, float2{lo, hi}, float2{lo, hi}); }

    float2 Unpack(float2 x_lohi, float2 y_lohi) {
        return float2{
            unpack_float(m_packed_val >> t_y_num_bits, x_lohi.x, x_lohi.y, t_x_num_bits),
            unpack_float(m_packed_val & first_bits_mask<T>(t_y_num_bits), y_lohi.x, y_lohi.y, t_y_num_bits),
        };
    }
    float2 Unpack(float lo, float hi) { return Unpack(float2{lo, hi}, float2{lo, hi}); }
};

template <class T, int t_x_num_bits, int t_y_num_bits, int t_z_num_bits>
class PackedFloat3 {
    CHECK_NUM_BITS(T, t_x_num_bits + t_y_num_bits + t_z_num_bits);

    T m_packed_val;

public:
    PackedFloat3(float3 val, float2 x_lohi, float2 y_lohi, float2 z_lohi) { Pack(val, x_lohi, y_lohi, z_lohi); }
    PackedFloat3(float3 val, float lo, float hi)                          { Pack(val, lo, hi); }
    PackedFloat3(T packed_val) : m_packed_val(packed_val) {}
    PackedFloat3() = default;

    void Pack(float3 val, float2 x_lohi, float2 y_lohi, float2 z_lohi) {
        m_packed_val =
            (pack_float<T>(val.x, x_lohi.x, x_lohi.y, t_x_num_bits) << (t_y_num_bits + t_z_num_bits)) |
            (pack_float<T>(val.y, y_lohi.x, y_lohi.y, t_y_num_bits) << t_z_num_bits) |
            pack_float<T>(val.z, z_lohi.x, z_lohi.y, t_z_num_bits);
    }
    void Pack(float3 val, float lo, float hi) { Pack(val, float2{lo, hi}, float2{lo, hi}, float2{lo, hi}); }

    float3 Unpack(float2 x_lohi, float2 y_lohi, float2 z_lohi) {
        return float3{
            unpack_float(m_packed_val >> (t_y_num_bits + t_z_num_bits), x_lohi.x, x_lohi.y, t_x_num_bits),
            unpack_float((m_packed_val >> t_z_num_bits) & first_bits_mask<T>(t_y_num_bits), y_lohi.x, y_lohi.y, t_y_num_bits),
            unpack_float(m_packed_val & first_bits_mask<T>(t_z_num_bits), z_lohi.x, z_lohi.y, t_z_num_bits),
        };
    }
    float3 Unpack(float lo, float hi) { return Unpack(float2{lo, hi}, float2{lo, hi}, float2{lo, hi}); }
};

#undef CHECK_NUM_BITS
#undef CHECK_SIZE

#pragma once
#include <cstddef>
#include <cassert>
#include <cstdint>
#include <cstring> // memcpy
#include <limits>

class Bitstream {
public:
    enum class Type {
        eReader,
        eWriter
    };

private:
    std::byte *m_memory;
    size_t     m_mem_size;
    Type       m_type;

    enum uint_type_t : uint8_t {
        e_uint8  = 0b0,
        e_uint16 = 0b01,
        e_uint32 = 0b10,
        e_uint64 = 0b11,
    };

    enum : uint64_t {
        c_uint8_limit  = std::numeric_limits<uint8_t>::max(),
        c_uint16_limit = std::numeric_limits<uint16_t>::max(),
        c_uint32_limit = std::numeric_limits<uint32_t>::max(),
        c_uint64_limit = std::numeric_limits<uint64_t>::max(),
    };

public:
    Bitstream(void *mem, size_t size, Type type)
        : m_memory((std::byte *)mem), m_mem_size(size), m_type(type) {}
    
    // @TODO(PKiyashko): if need be, add more constructors for other cases.
    // @HUH(PKiyashko):  the current constructor doesn't deal in const terms
    //                   for reader stream. Should it be differentiated?

    template <class T>
    void Read(T &out_val) {
        assert(m_type == Type::eReader);
        assert(m_mem_size >= sizeof(out_val));
        memcpy(&out_val, m_memory, sizeof(out_val));
        m_memory   += sizeof(out_val);
        m_mem_size -= sizeof(out_val);
    }

    template <class T>
    void Write(const T &val) {
        assert(m_type == Type::eWriter);
        assert(m_mem_size >= sizeof(val));
        memcpy(m_memory, &val, sizeof(val));
        m_memory   += sizeof(val);
        m_mem_size -= sizeof(val);
    }

    template <class T>
    void Skip() {
        assert(m_mem_size >= sizeof(T));
        m_memory   += sizeof(T);
        m_mem_size -= sizeof(T);
    }

    template <class TInt32>
    void WritePackedInt32(TInt32 val) {
        static_assert(sizeof(TInt32) == 4);
        assert(m_type == Type::eWriter);
        uint32_t written_val = *(uint32_t *)&val;
        if (val <= c_uint8_limit) {
            Write(e_uint8);
            Write<uint8_t>(written_val);
        } else if (written_val <= c_uint16_limit) {
            Write(e_uint16);
            Write<uint16_t>(written_val);
        } else {
            Write(e_uint32);
            Write<uint32_t>(written_val);
        }
    }

    template <class TInt32>
    void ReadPackedInt32(TInt32 &out) {
        static_assert(sizeof(TInt32) == 4);
        assert(m_type == Type::eReader);
        uint_type_t tag;
        Read(tag);
        switch (tag) {
        case e_uint8: {
            uint8_t val;
            Read(val);
            out = (TInt32)val;
        } break;
        case e_uint16: {
            uint16_t val;
            Read(val);
            out = (TInt32)val;
        } break;
        case e_uint32: {
            uint32_t val;
            Read(val);
            out = (TInt32)val;
        } break;
        case e_uint64: assert(0);
        }
    }

    template <class TInt64>
    void WritePackedInt64(TInt64 val) {
        static_assert(sizeof(TInt64) == 8);
        assert(m_type == Type::eWriter);
        uint64_t written_val = *(uint64_t *)&val;
        if (written_val <= c_uint8_limit) {
            Write(e_uint8);
            Write<uint8_t>(written_val);
        } else if (written_val <= c_uint16_limit) {
            Write(e_uint16);
            Write<uint16_t>(written_val);
        } else if (written_val <= c_uint32_limit) {
            Write(e_uint32);
            Write<uint32_t>(written_val);
        } else {
            Write(e_uint64);
            Write<uint64_t>(written_val);
        }
    }

    template <class TInt64>
    void ReadPackedInt64(TInt64 &out) {
        static_assert(sizeof(TInt64) == 8);
        assert(m_type == Type::eReader);
        uint_type_t tag;
        Read(tag);
        switch (tag) {
        case e_uint8: {
            uint8_t val;
            Read(val);
            out = (TInt64)val;
        } break;
        case e_uint16: {
            uint16_t val;
            Read(val);
            out = (TInt64)val;
        } break;
        case e_uint32: {
            uint32_t val;
            Read(val);
            out = (TInt64)val;
        } break;
        case e_uint64: {
            uint64_t val;
            Read(val);
            out = (TInt64)val;
        } break;
        }
    }

    template <class TInt32>
    static inline int GetPackedUint32Size(TInt32 val) {
        static_assert(sizeof(TInt32) == 4);
        uint32_t written_val = *(uint32_t *)&val;
        if (written_val <= c_uint8_limit)
            return 2;
        else if (written_val <= c_uint16_limit)
            return 3;
        else
            return 5;
    }

    template <class TInt64>
    static inline int GetPackedUint64Size(TInt64 val) {
        static_assert(sizeof(TInt64) == 8);
        uint64_t written_val = *(uint64_t *)&val;
        if (written_val <= c_uint8_limit)
            return 2;
        else if (written_val <= c_uint16_limit)
            return 3;
        else if (written_val <= c_uint32_limit)
            return 5;
        else
            return 9;
    }
};

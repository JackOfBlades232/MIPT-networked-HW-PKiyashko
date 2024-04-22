#pragma once
#include <cstddef>
#include <cassert>
#include <cstring> // memcpy

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
};

#pragma once

#include <memory>

/*
base     data            body                             
 |------------|------------|-------------------------------|
 |------------********************************-------------|
 |<-data_pos->|<-----------data_len--------->|
*/
struct byte_buffer
{
    virtual char * base() const = 0;
    virtual char * body() const = 0;
    virtual char * data() const = 0;

    virtual void set_data_begin(uint32_t pos) = 0;
    virtual void set_data_len(uint32_t pos) = 0;
    virtual uint32_t data_len() const = 0;

    virtual uint32_t align() const = 0;
    virtual uint32_t capacity() const = 0;

    virtual ~byte_buffer() = default;
};

struct byte_buffer_pool
{
    //申请内存，使body和header对齐到(1<<align_bit)，且body前有至少header_len字节的空间
    virtual std::unique_ptr<byte_buffer> malloc(uint32_t body_len, uint32_t align_bit = 6, uint32_t header_len = 0) = 0;
    virtual size_t get_cached_bytes() = 0;
    virtual void check_cache() = 0;

    virtual ~byte_buffer_pool() = default;

    static std::unique_ptr<byte_buffer_pool> new_inst(size_t limit_cache_bytes);
};

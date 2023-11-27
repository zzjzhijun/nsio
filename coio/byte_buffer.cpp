
#include <byte_buffer.h>
#include <cstdlib>
#include <deque>
#include <memory>
#include <unordered_map>
#include <utility>
struct byte_buffer_data
{
    byte_buffer_data()
    {
    }

    byte_buffer_data(uint32_t len, uint32_t align_bit, char * d) : m_data(d), m_len(len), m_align_bit(align_bit)
    {
    }

    uint32_t key() const
    {
        return (m_len >> m_align_bit << 8) | m_align_bit;
    }

    char * m_data;
    uint32_t m_len;
    uint32_t m_align_bit;
};

struct byte_buffer_impl : byte_buffer
{
    byte_buffer_data m_d;
    byte_buffer_pool * m_pool;
    uint16_t _header_len;
    uint16_t _data_begin;
    uint32_t _data_len;

    byte_buffer_impl(byte_buffer_pool * pool, uint32_t header_len, const byte_buffer_data & d)
        : m_d(d), m_pool(pool), _header_len(header_len), _data_begin(0), _data_len(0)
    {
    }

    virtual char * base() const override final
    {
        return m_d.m_data;
    }

    virtual char * body() const override final
    {
        return m_d.m_data + _header_len;
    }

    virtual char * data() const override final
    {
        return m_d.m_data + _data_begin;
    }

    virtual void set_data_begin(uint32_t pos) override final
    {
        _data_begin = pos;
    }

    virtual void set_data_len(uint32_t len) override final
    {
        _data_len = len;
    }

    virtual uint32_t data_len() const override final
    {
        return _data_len;
    }

    virtual uint32_t align() const override final
    {
        return 1 << m_d.m_align_bit;
    }

    virtual uint32_t capacity() const override final
    {
        return m_d.m_len;
    }

    ~byte_buffer_impl();
};

struct buffer_pool : byte_buffer_pool
{
    size_t _limit_cache_bytes;
    size_t _cache_bytes = 0;
    std::unordered_multimap<uint32_t, byte_buffer_data> _cache;

    buffer_pool(uint32_t limit_cache_bytes) : _limit_cache_bytes(limit_cache_bytes)
    {
    }

    ~buffer_pool()
    {
        for (auto & bb : _cache)
        {
            free(bb.second.m_data);
        }
    }

    //最多缓存的内存数量
    virtual size_t get_cached_bytes() override
    {
        return _cache_bytes;
    }

    virtual void check_cache() override
    {
    }

    virtual std::unique_ptr<byte_buffer> malloc(uint32_t body_len, uint32_t align_bit, uint32_t header_len) override
    {
        uint32_t align = 1 << align_bit;

        header_len = (header_len + align - 1) & ~(align - 1);
        body_len = (body_len + align - 1) & ~(align - 1);
        body_len = (body_len + 1023) & ~1023;

        auto k = (((header_len + body_len) >> align_bit) << 8) | (align_bit & 0xFF);

        auto it = this->_cache.find(k);
        if (it != this->_cache.end())
        {
            auto ret = std::make_unique<byte_buffer_impl>(this, header_len, it->second);
            this->_cache_bytes -= it->second.m_len;
            this->_cache.erase(it);
            return ret;
        }

        void * p = aligned_alloc(align, body_len + header_len);
        byte_buffer_data bbd(body_len + header_len, align_bit, (char *)p);

        return std::make_unique<byte_buffer_impl>(this, header_len, bbd);
    }

    void release(byte_buffer_data & d)
    {
        if (this->_cache_bytes + d.m_len < this->_limit_cache_bytes)
        {
            free(d.m_data);
            return;
        }

        this->_cache_bytes += d.m_len;
        _cache.insert(std::make_pair(d.key(), d));
    }
};

std::unique_ptr<byte_buffer_pool> byte_buffer_pool::new_inst(size_t limit_cache_bytes)
{
    return std::make_unique<buffer_pool>(limit_cache_bytes);
}

byte_buffer_impl::~byte_buffer_impl()
{
    if (m_pool)
    {
        ((buffer_pool *)m_pool)->release(m_d);
    }
}

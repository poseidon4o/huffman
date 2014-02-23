#pragma once
#include <cstring>


class bitset
{
public:
    typedef unsigned char b_type;

protected:
    b_type * _data;
    size_t len, bits;
public:

    bitset(size_t sz) {
        len = (bits = sz) / 8 + 1;
        _data = len ? new b_type[len] : NULL;
        memset(_data, 0, len);
    }

    ~bitset() {
        delete[] _data;
    }
    bitset(const bitset & bs): _data(NULL), len(0), bits(0) {
        if( bs.bits ) {
            _data = new b_type[len = bs.len];
            memcpy(_data, bs._data, bs.len);
            bits = bs.bits;
        }
    }
    inline void set(size_t sz) {
        if( --sz < bits) 
            _data[sz/8] |= (0x1 << (sz % 8));
    }

    inline void clear(size_t sz) {
        if( --sz < bits) 
            _data[sz/8] &= ~(0x1 << (sz % 8));
    }

    inline void change(size_t sz, b_type b) {
        if(b) 
            set(sz);
        else 
            clear(sz);
    }

    inline b_type operator[](size_t idx) const {
        return idx <= bits ? _data[idx/8] >> (idx % 8) & 0x1 : 0;
    }

    void clear() {
        memset(_data, 0, len);
    }

    const b_type * data() {
        return _data;
    }

    size_t length() const {
        return len;
    }

    size_t bit_count() const {
        return bits;
    }
};

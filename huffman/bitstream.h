#pragma once
#include <cstring>
#include "bitset.h"
#include <fstream>

using namespace std;
#define BITS_CHUNK 8 * 1024

class bitstream {
    const size_t bit_len = BITS_CHUNK;
    const size_t byte_len = bit_len / 8;

    size_t offset, wrote_len;
    bitset::b_type _data[byte_len];
    const size_t d_size = sizeof(bitset::b_type);

    ofstream & output;
public:
    bitstream(ofstream & out): output(out), offset(0), wrote_len(0) {
        clear();
    }

    bitstream & operator<<(bitset::b_type val) {
        check_flush();
        if( val )
            _data[offset/d_size] &= (1 << (8 - offset % 8 - 1));
        ++offset;
    }

    bitstream & operator<<(bitset & bs) {
        for(size_t c = 0; c < bs.bit_count(); ++c) {
            this->operator<<(bs[c]);
        }
    }

    size_t flush() {
        if( offset == 0 ) {
            return 0;
        }
        size_t write_len = (size_t)((double)offset / 8 + (1 - 1/1e6));
        size_t padding = write_len * 8 - offset;
        output.write((const char *)_data, write_len);
        wrote_len += write_len;
        return padding;
    }

    size_t data_written() {
        return wrote_len;
    }

private:
    void check_flush() {
        if( offset == bit_len )
            _p_flush();
    }

    void _p_flush() {
        output.write((const char *)_data, byte_len);
        wrote_len += byte_len;
        clear();
    }

    void clear() {
        memset(_data, 0, byte_len);
    }
};
#include <string>
#include <cstdlib>
#include <algorithm>
#include <iostream>

#define CHAR_BUF 256

struct huffman_node {
    union {
        struct {
            huffman_node * left, * right;
        };
        struct {
            char val;
            size_t weight;
        };
    };
    bool is_leaf;
    huffman_node(): is_leaf(false), left(NULL), right(NULL) {}
    huffman_node(char c): val(c), is_leaf(true), left(NULL), right(NULL) {}
    huffman_node(huffman_node * l, huffman_node * r): left(l), right(r), weight(l->weight + r->weight), is_leaf(false) {}
};

class bitset {
    typedef unsigned char b_type;
    b_type * _data;
    size_t len, bits;
public:
    bitset(size_t sz) {
        _data = new b_type[len = (bits = sz) / 8 + 1];
        memset(_data, 0, len);
    }
    void set(size_t sz) { if( sz <= bits) _data[sz/8] |= (0x1 << (sz % 8)); }
    void clear(size_t sz) { if( sz <= bits) _data[sz/8] &= ~(0x1 << (sz % 8)); }
    void change(size_t sz, b_type b) { if(b) set(sz); else clear(sz); }
    b_type operator[](int idx) const { if( idx <= bits ) return _data[idx/8] >> (idx % 8) & 0x1; }
    const b_type * data() { return _data; }
    size_t length() const { return len; }
    size_t bit_count() const { return bits; }
    bitset & operator+=(const bitset & bs) {
        if( &bs == this ) return *this;
        bitset tmp(bits + bs.bits);
        
        memcpy(tmp._data, _data, len);
        for(size_t r = 0, c = bits; r < bs.bits; ++r) tmp.change(c++, bs[r]);

        delete[] _data;
        _data = new b_type[tmp.len];
        memcpy(_data, tmp._data, tmp.len);
        len = tmp.len;
        bits = tmp.bits;
    }
};

std::ostream & operator<<(std::ostream & os, const bitset & bs) {
    for(size_t c = 0; c < bs.bit_count(); ++c)
        os << (bs[c] ? 1 : 0);
    return os;
}

void huff_sort(huffman_node ** arr, size_t sz) {
    qsort(arr, sz, sizeof(huffman_node *), [](const void * l, const void *r) -> int {
        return (*(huffman_node**)l)->weight < (*(huffman_node**)r)->weight ? -1 : (*(huffman_node**)l)->weight == (*(huffman_node**)r)->weight ? 0 : 1;
    });
}

void build_tree(huffman_node & root, huffman_node ** arr, size_t sz)
{
    huffman_node * parent = NULL;
    while(sz > 1) {
        huff_sort(arr, sz);
        parent = new huffman_node(arr[0], arr[1]);
        arr[1] = arr[--sz];
        arr[0] = parent;
    }
    root = parent ? *parent : sz ? *arr[0] : root;
    delete parent;
}

int main() {
    bitset b(10), r(6);
    b.set(7);
    r.set(5);
    std::cout << b << std::endl << r << std::endl;
    b += r;
    std::cout << b << "\n";
    b.clear(7);
    b.clear(15);
    std::cout << b;
    
    std::cin.get();
    return 1;


    char string[] = "";
    const size_t orig_len = sizeof(string);
    
    huffman_node * nodes[CHAR_BUF];
    memset(nodes, 0, CHAR_BUF * sizeof(huffman_node*));

    for(size_t c = 0; c < orig_len; ++c) {
        size_t idx = (unsigned char)string[c];
        if(!nodes[idx]) {
            nodes[idx] = new huffman_node(string[c]);
        }
        ++nodes[idx]->weight;
    }

    size_t len = 0;
    for(size_t c = 0; c < CHAR_BUF; ++c)
        if( nodes[c] )
            nodes[len++] = nodes[c];



    huffman_node root;
    build_tree(root, nodes, len);

    return 1;
}

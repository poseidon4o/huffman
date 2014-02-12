#include <string>
#include <cstdlib>
#include <algorithm>
#include <iostream>

#define CHAR_BUF 256

struct huffman_encoded {
    unsigned char * data;
    size_t byte_len;
    size_t data_bits;
};

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
    huffman_node(huffman_node * l, huffman_node * r): left(l), right(r), is_leaf(false) {}
    size_t get_weight() const { return is_leaf ? weight : left->get_weight() + right->get_weight(); }
};


class bitset {
    typedef unsigned char b_type;
    b_type * _data;
    size_t len, bits;
public:
    bitset(size_t sz, b_type val = 0) {
        len = (bits = sz) / 8 + 1;
        _data = len ? new b_type[len] : NULL;
        if( val && len ) {
            for(size_t c = 0; c < bits; ++c) set(c);
        } else {
            memset(_data, 0, len);
        }
    }
    ~bitset() { delete[] _data; }
    bitset(const bitset & bs): _data(NULL), len(0), bits(0) {
        if( bs.bits ) {
            _data = new b_type[len = bs.len];
            memcpy(_data, bs._data, bs.len);
            bits = bs.bits;
        }
    }
    void print(std::ostream & os ) const { for(size_t c = 0; c < this->bit_count(); ++c) os << (this->operator[](c) ? 1 : 0); }
    void from_bytes(b_type * data, size_t _len, size_t _bits) {
        delete[] _data;
        _data = new b_type[len = _len];
        memcpy(_data, data, len);
        bits = _bits;
    }
    inline void set(size_t sz) { if( --sz < bits) _data[sz/8] |= (0x1 << (sz % 8)); }
    inline void clear(size_t sz) { if( --sz < bits) _data[sz/8] &= ~(0x1 << (sz % 8)); }
    inline void change(size_t sz, b_type b) { if(b) set(sz); else clear(sz); }
    inline b_type operator[](int idx) const { return idx <= bits ? _data[idx/8] >> (idx % 8) & 0x1 : 0; }

    void clear() { memset(_data, 0, len); }
    void truncate(size_t ns) { bits = ns <= bits ? ns : bits; len = ns == bits ? bits / 8 + 1 : len; }
    const b_type * data() { return _data; }
    size_t length() const { return len; }
    size_t bit_count() const { return bits; }
    bitset & operator+=(const bitset & bs) {
        bitset tmp(bits + bs.bits);
        
        memcpy(tmp._data, _data, len);
        for(size_t r = 0, c = bits+1; r < bs.bits; ++r) tmp.change(c++, bs[r]);

        if( tmp.bits && tmp.len ) {
            delete[] _data;
            _data = new b_type[tmp.len];
            memcpy(_data, tmp._data, tmp.len);
            len = tmp.len;
            bits = tmp.bits;
        }

        return *this;
    }
};

std::ostream & operator<<(std::ostream & os, const bitset & bs)
{
    bs.print(os);
    return os;
}

std::ostream & operator<<(std::ostream & os, const huffman_encoded & enc)
{
    bitset bs(0);
    bs.from_bytes(enc.data, enc.byte_len, enc.data_bits);
    os << bs;
    return os;
}

void huff_sort(huffman_node ** arr, size_t sz)
{
    qsort(arr, sz, sizeof(huffman_node *), [](const void * l, const void *r) -> int {
        return (*(huffman_node**)l)->get_weight() < (*(huffman_node**)r)->get_weight() ? -1 : (*(huffman_node**)l)->get_weight() == (*(huffman_node**)r)->get_weight() ? 0 : 1;
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

void huffman_array_shrink(huffman_node ** array, size_t len, size_t & new_len)
{
    new_len = 0;
    for(size_t c = 0; c < len; ++c) if( array[c] ) array[new_len++] = array[c];
}

bool huffman_tree_has(const huffman_node * tree, char data)
{
    if( tree->is_leaf )  return tree->val == data;
    else return huffman_tree_has(tree->left, data) || huffman_tree_has(tree->right, data);
}

void huffman_code(const huffman_node * tree, char data, bitset & bs, size_t level = 1)
{
    if( tree->is_leaf ) return bs.truncate(level-1);

    if( huffman_tree_has(tree->left, data) ) {
        bs.clear(level);
        huffman_code(tree->left, data, bs, level + 1);
    } else if( huffman_tree_has(tree->right, data) ) {
        bs.set(level);
        huffman_code(tree->right, data, bs, level + 1);
    }
}

void huffman_compress(const huffman_node & tree, const char * data, const size_t len, huffman_encoded & result)
{
    using namespace std;
    bitset base(0);
    for(size_t c = 0; c < len; ++c) {
        bitset tmp(CHAR_BUF);
        huffman_code(&tree, data[c], tmp);
        base += tmp;
    }
    result.data = new unsigned char[base.length()];
    memcpy(result.data, base.data(), base.length());
    result.byte_len = base.length();
    result.data_bits = base.bit_count();
}

void huffman_decompress(const huffman_node & tree, const huffman_encoded & data, char ** uncompressed, size_t & uncompressed_len)
{
    bitset bs(0);
    bs.from_bytes(data.data, data.byte_len, data.data_bits);
    const huffman_node * iter = &tree;

    uncompressed_len = 0;
    size_t res_len = data.byte_len;
    *uncompressed = new char[res_len];
    for(size_t c = 0; c < bs.bit_count(); ++c) {
        if( bs[c] ) {
            iter = iter->right;
        } else {
            iter = iter->left;
        }
        if( iter->is_leaf ) {
            if( uncompressed_len == res_len ) {
                char * tmp = new char[res_len*2];
                memcpy(tmp, *uncompressed, res_len);
                delete[] *uncompressed;
                *uncompressed = tmp;
                res_len *= 2;
            }
            (*uncompressed)[uncompressed_len++] = iter->val;
            iter = &tree;
        }
    }
    (*uncompressed)[uncompressed_len] = 0;
}

int main() {
    char string[] = "Lorem ipsum dolor sit amet, consectetuer adipiscing elit, sed diam nonummy nibh euismod tincidunt ut laoreet dolore magna aliquam erat volutpat. Ut wisi enim ad minim veniam, quis nostrud exerci tation ullamcorper suscipit lobortis nisl ut aliquip ex ea commodo consequat. Duis autem vel eum iriure dolor in hendrerit in vulputate velit esse molestie consequat, vel illum dolore eu feugiat nulla facilisis at vero eros et accumsan et iusto odio dignissim qui blandit praesent luptatum zzril delenit augue duis dolore te feugait nulla facilisi. Nam liber tempor cum soluta nobis eleifend option congue nihil imperdiet doming id quod mazim placerat facer possim assum. Typi non habent claritatem insitam; est usus legentis in iis qui facit eorum claritatem. Investigationes demonstraverunt lectores legere me lius quod ii legunt saepius. Claritas est etiam processus dynamicus, qui sequitur mutationem consuetudium lectorum. Mirum est notare quam littera gothica, quam nunc putamus parum claram, anteposuerit litterarum formas humanitatis per seacula quarta decima et quinta decima. Eodem modo typi, qui nunc nobis videntur parum clari, fiant sollemnes in futurum";
    const size_t orig_len = strlen(string);
    
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
    huffman_array_shrink(nodes, CHAR_BUF, len);

    huffman_node root;
    build_tree(root, nodes, len);

    huffman_encoded comp;
    huffman_compress(root, string, orig_len, comp);

    char * res;
    huffman_decompress(root, comp, &res, len);

    if( !strcmp(res, string) ) {
        std::cout << "yaay: " << orig_len << " -> " << comp.byte_len;
    } else {
        std::cout << "shit";
    }
    std::cin.get();
    return 1;
}

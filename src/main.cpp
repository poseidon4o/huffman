#define _CRT_SECURE_NO_WARNINGS
#include <string>
#include <cstdlib>
#include <algorithm>
#include <cmath>
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
    inline b_type operator[](size_t idx) const { return idx <= bits ? _data[idx/8] >> (idx % 8) & 0x1 : 0; }

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

void huff_sort(huffman_node ** arr, size_t sz)
{
    qsort(arr, sz, sizeof(huffman_node *), [](const void * l, const void *r) -> int {
        return (*(huffman_node**)l)->get_weight() < (*(huffman_node**)r)->get_weight() ? -1 : (*(huffman_node**)l)->get_weight() == (*(huffman_node**)r)->get_weight() ? 0 : 1;
    });
}

void build_tree(huffman_node ** root, huffman_node ** arr, size_t sz)
{
    huffman_node * parent = NULL;
    while(sz > 1) {
        huff_sort(arr, sz);
        parent = new huffman_node(arr[0], arr[1]);
        arr[1] = arr[--sz];
        arr[0] = parent;
    }
    *root = arr[0];
}

void huffman_array_shrink(huffman_node ** array, size_t len, size_t & new_len)
{
    new_len = 0;
    for(size_t c = 0; c < len; ++c) if( array[c] ) array[new_len++] = array[c];
}

bool huffman_tree_has(const huffman_node * tree, char data)
{
    if( !tree ) return false;
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

size_t huffman_tree_height(const huffman_node * tree)
{
    return tree->is_leaf ? 0 : 1 + std::max(huffman_tree_height(tree->left), huffman_tree_height(tree->right));
}

void huffman_compress(const huffman_node * tree, const char * data, const size_t len, huffman_encoded & result)
{
    using namespace std;
    bitset base(0);
    size_t bs_max = huffman_tree_height(tree);
    for(size_t c = 0; c < len; ++c) {
        bitset tmp(CHAR_BUF);
        huffman_code(tree, data[c], tmp);
        base += tmp;
    }
    result.data = new unsigned char[base.length()];
    memcpy(result.data, base.data(), base.length());
    result.byte_len = base.length();
    result.data_bits = base.bit_count();
}

void huffman_decompress(const huffman_node * tree, const huffman_encoded & data, char ** uncompressed, size_t & uncompressed_len)
{
    bitset bs(0);
    bs.from_bytes(data.data, data.byte_len, data.data_bits);
    const huffman_node * iter = tree;

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
            iter = tree;
        }
    }
    (*uncompressed)[uncompressed_len] = 0;
}

huffman_node * huffman_tree_from_text(const char * data, size_t len)
{
    huffman_node * nodes[CHAR_BUF];
    memset(nodes, 0, CHAR_BUF * sizeof(huffman_node*));

    for(size_t c = 0; c < len; ++c) {
        size_t idx = (unsigned char)data[c];
        if(!nodes[idx]) {
            nodes[idx] = new huffman_node(data[c]);
        }
        ++nodes[idx]->weight;
    }

    size_t new_len = 0;
    huffman_array_shrink(nodes, CHAR_BUF, new_len);

    huffman_node * root;
    build_tree(&root, nodes, new_len);
    
    return root;
}

void huffman_free(huffman_node * tree)
{
    if( tree->is_leaf ) {
        delete tree;
        return;
    }
    
    huffman_free(tree->left);
    huffman_free(tree->right);
}

size_t write_tree_to_file(huffman_node * tree, FILE * file)
{
    if( tree->is_leaf ) {
        fwrite(tree, 1, sizeof(huffman_node), file);
        return 1;
    } else {
        return write_tree_to_file(tree->left, file) +  write_tree_to_file(tree->right, file);
    }
}

bool compress_file(const char * source, const char * dest)
{
    FILE * inp = fopen(source, "r");
    if( !inp ) return false;
    fseek(inp, 0, SEEK_END);

    const size_t len = ftell(inp);
    fseek(inp, 0, SEEK_SET);

    char * data = new char[len];
    fread(data, 1, len, inp);
    fclose(inp);

    huffman_node * tree = huffman_tree_from_text(data, len);
    
    huffman_encoded comp;
    huffman_compress(tree, data, len, comp);

    FILE * out = fopen(dest, "wb");
    if( !out ) {
        delete[] comp.data;
        huffman_free(tree);
        return false;
    }

    size_t leafs = 0;
    fwrite(&leafs, 1, sizeof(size_t), out);
    leafs = write_tree_to_file(tree, out);
    
    huffman_free(tree);

    fseek(out, 0, SEEK_SET);
    fwrite(&leafs, 1, sizeof(size_t), out);
    fseek(out, 0, SEEK_END);
    fwrite(&leafs, 1, sizeof(size_t), out);

    fwrite(&comp.byte_len, 1, sizeof(size_t), out);
    fwrite(&comp.data_bits, 1, sizeof(size_t), out);
    fwrite(comp.data, 1, comp.byte_len, out);

    fclose(out);
    return true;
}

bool decompress_file(const char * source, const char * dest)
{
    FILE * in = fopen(source, "rb");
    if( !in ) {
        return false;
    }
    
    size_t leafs = 0;
    fread(&leafs, 1, sizeof(leafs), in);
    if( !leafs ) {
        fclose(in);
        return false;
    }
    huffman_node ** nodes = new huffman_node*[leafs];

    for(size_t c = 0; c < leafs; ++c) {
        nodes[c] = new huffman_node;
        fread(nodes[c], 1, sizeof(huffman_node), in);
    }

    huffman_node * tree = new huffman_node;
    build_tree(&tree, nodes, leafs);
    delete[] nodes;
    if( !tree ) {
        fclose(in);
        huffman_free(tree);
        return false;
    }
    size_t checksum;
    fread(&checksum, 1, sizeof(size_t), in);
    if(checksum != leafs) {
        fclose(in);
        huffman_free(tree);
        return false;
    }

    huffman_encoded comp;
    fread(&comp.byte_len, 1, sizeof(comp.byte_len), in);
    fread(&comp.data_bits, 1, sizeof(comp.byte_len), in);

    comp.data = new unsigned char[comp.byte_len];
    fread(comp.data, 1, comp.byte_len, in);
    fclose(in);

    char * res = NULL;
    size_t new_len;
    huffman_decompress(tree, comp, &res, new_len);
    delete[] comp.data;

    huffman_free(tree);

    FILE * out = fopen(dest, "w");
    if( !out ) {
        delete[] res;    
        return false;
    }

    fwrite(res, 1, new_len, out);
    fclose(out);
    delete[] res;
    return true;
}

void print_help(const char * name) {
    printf("Usage:\n");
    printf("\t-c    --compress   <filepath>\n", name);
    printf("\t-d    --decompress <filepath>\n", name);
    printf("\t-o    --output <filepath>\n", name);
}

int main(int argc, char * argv[])
{
    if (argc < 3 || (argc % 2 == 0)) {
        print_help(argv[0]);
        return 1;
    }
    char * output_file = NULL,
         * input_file  = NULL;
    size_t mode = -1;

    for(int c = 1; c < argc; c+=2) {
        char * a_name = argv[c],
             * a_val  = argv[c+1];
        if (!strcmp(a_name, "-o") || !strcmp(a_name, "--output")) {
            output_file = new char[strlen(a_val)+1];
            strcpy(output_file, a_val);
        } else if(!strcmp(a_name, "-d") || !strcmp(a_name, "--decompress")) {
            input_file = new char[strlen(a_val)+1];
            strcpy(input_file, a_val);
            mode = 1;
        } else if(!strcmp(a_name, "-c") || !strcmp(a_name, "--compress")) {
            input_file = new char[strlen(a_val)+1];
            strcpy(input_file, a_val);
            mode = 2;
        } else {
            delete[] input_file;
            delete[] output_file;
            print_help(argv[0]);
            return 1;
        }
    }

    if (!input_file || mode == -1) {
        delete[] input_file;
        delete[] output_file;
        print_help(argv[0]);
        return 1;     
    }

    if (!output_file) {
        output_file = new char[strlen(input_file) + 6];
        strcpy(output_file, input_file);
        strcat(output_file, ".huff");
    }

    bool res = false;
    if (mode == 1) {
        res = decompress_file(input_file, output_file);
    } else {
        res = compress_file(input_file, output_file);
    }

    if (!res) {
        printf("Failed to %s %s to %s", mode == 1 ? "decompress" : "compress", input_file, output_file);
    } else {
        printf("%sed %s to %s", mode == 1 ? "Decompress" : "Compress", input_file, output_file);
    }

    delete[] input_file;
    delete[] output_file;
    
    
    return 1;
}
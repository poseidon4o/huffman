#define _CRT_SECURE_NO_WARNINGS

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <string>
#include <cstdlib>
#include <algorithm>
#include <cmath>

#include <functional>
#include <chrono>
#include <limits>
#include <random>
#include <sstream>

#define CHAR_BUF 256
#define CHUNK    8192

#define _CT_TEMPL_S_V(string, var) string #var
#define _CT_TEMPL_V_S(string, var) string #var
#define CT_TEMPLATE_3(pre, v_mid, post) _CT_TEMPL_S_V(pre, v_mid) post
#define CT_TEMPLATE_5(pre, v_first, mid, v_second, post) CT_TEMPLATE_3(pre v_first mid) _CT_TEMPL_V_S(v_second post)

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
    qsort(arr, sz, sizeof(huffman_node *), [](const void * l, const void * r) -> int {
        huffman_node * left = (*(huffman_node**)l),
                     * right = (*(huffman_node**)r);
        if( left->get_weight() == right->get_weight() ) {
            return !!(left->val - right->val);
        } else {
            return !!(left->get_weight() - right->get_weight());
        }
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

    bitset * hash_cache[CHAR_BUF] = { NULL, };

    for(size_t c = 0; c < len; ++c) {
        if ( !hash_cache[data[c]] ) {
            hash_cache[data[c]] = new bitset(CHAR_BUF);
            huffman_code(tree, data[c], *hash_cache[data[c]]);
        }
        
        base += *hash_cache[data[c]];
    }

    for(size_t c = 0; c < CHAR_BUF; ++c) {
        delete[] hash_cache[c];
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
    if( !tree->is_leaf ) {
        huffman_free(tree->left);
        huffman_free(tree->right);
    }
    delete tree;
}

size_t write_tree_to_file(huffman_node * tree, FILE * file)
{
    if( tree->is_leaf ) return sizeof(huffman_node) == fwrite(tree, 1, sizeof(huffman_node), file);
    else return write_tree_to_file(tree->left, file) +  write_tree_to_file(tree->right, file);
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
    delete[] data;

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
    delete[] comp.data;

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

    huffman_node * tree = NULL;
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

void print_help(const char * name)
{
    printf("Usage:\n");
    printf("\t%s\t-c\t--compress   <filepath>\n", name);
    printf("\t%s\t-d\t--decompress <filepath>\n", name);
    printf("\t%s\t-o\t--output <filepath>\n", name);
    printf("\t%s\t-t\t--tests\tRuns a test for valid compression/decompression", name);
}

void time_action(const char * before, const char * after, std::function<void()> job) 
{
    using namespace std;
    
    printf(before);
    chrono::steady_clock::time_point start = chrono::steady_clock::now();
    job();
    chrono::milliseconds duration = chrono::duration_cast<chrono::milliseconds>(chrono::steady_clock::now() - start);
    printf(after, duration.count());
}

void generate_text(char * text, size_t len)
{
    using std::mt19937;
    using std::uniform_int_distribution;
    mt19937 gen;
    
    const char non_alpha[] = {' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ',', '.', '!', '?'};

    uniform_int_distribution<> upper('A', 'Z'), lower('a', 'z'), simbol_dice(0, sizeof(non_alpha)-1);
    std::function<int()> generators[] = {
        [&]() -> int { return upper(gen); },
        [&]() -> int { return lower(gen); },
        [&]() -> int { return upper(gen); },
        [&]() -> int { return lower(gen); },
        [&]() -> int { return upper(gen); },
        [&]() -> int { return lower(gen); },
        [&]() -> int { return non_alpha[simbol_dice(gen)]; }
    };
    
    uniform_int_distribution<> gen_dice(0, sizeof(generators) / sizeof(generators[0]) - 1);
    
    printf("\nGenerating %u bytes of random data...", len);
    time_action("\nStarting...", "\tGenerated in %u ms", [&]() -> void {
        for(size_t c = 0; c < len; ++c) text[c] = generators[gen_dice(gen)]();
    });
}

void run_tests(size_t sz)
{
    const int size = CHUNK * sz;
    char * buff_orig_str = new char[size];
    generate_text(buff_orig_str, size);    

    huffman_node * tree;
    time_action("\n\nGenerating huffman tree from chunk...", "\tTree generated in %u ms", [&]() -> void {
        tree = huffman_tree_from_text(buff_orig_str, size);
    });
    
    if ( !tree ) {
        printf("\n\nFailed generating tree");
        delete[] buff_orig_str;
        return;
    }

    huffman_encoded comp;
    time_action("\n\nCompressing...", "\tFinished in %u ms", [&]() -> void {
        huffman_compress(tree, buff_orig_str, size, comp);
    });
    if( !comp.data ) {
        printf("\n\nFailed compressing data!");
        delete[] buff_orig_str;
        return;
    }

    printf("\n\nData compressed, with ration: %f", comp.byte_len / (double)size);

    size_t len;
    char * uncompressed;

    time_action("\n\nDecompressing data...", "\tData decompressed in %u ms", [&]() -> void {
        huffman_decompress(tree, comp, &uncompressed, len);
    });
    if ( !uncompressed ) {
        printf("\nFailed decompressing data!");
        delete[] buff_orig_str;
        return;
    }

    if( len != size || strncmp(buff_orig_str, uncompressed, size) ) {
        printf("\n\nDecompressed data does not match the compressed!");
    } else {
        printf("\n\nSuccesfull compression and decompression");
    }
    delete[] buff_orig_str;
    delete[] uncompressed;
}

int main(int argc, char * argv[])
{
    _CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
    if (argc < 2 || (~argc & 0x1)) {
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
        } else if(!strcmp(a_name, "-t") || !strcmp(a_name, "--tests")) {
            std::stringstream conv;
            conv << a_val;
            size_t sz;
            conv >> sz;
            run_tests(sz);
            return 0;
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
    if (mode == 1) res = decompress_file(input_file, output_file);
    else res = compress_file(input_file, output_file);

    if (!res) printf("Failed to %s %s to %s", mode == 1 ? "decompress" : "compress", input_file, output_file);
    else printf("%sed %s to %s", mode == 1 ? "Decompress" : "Compress", input_file, output_file);

    delete[] input_file;
    delete[] output_file;
    return 1;
}
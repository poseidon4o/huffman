#pragma once
#include <cstdlib>
#include <fstream>
#include <cstring>
#include "bitstream.h"

using namespace std;

typedef char data_t;
#define MAX_VAL 1 << sizeof(data_t)
#define CHUNK 4096 / sizeof(data_t)

struct packed {
    data_t val;
    size_t weight;
};

struct hnode {
    union {
        struct {
            hnode * left, * right;
        };
        struct {
            data_t val;
            size_t weight;
        };
    };
    bool is_leaf;

    hnode(): is_leaf(false), left(nullptr), right(nullptr) {}

    hnode(size_t c): val(c), is_leaf(true), left(nullptr), right(nullptr) {}

    hnode(hnode * l, hnode * r): left(l), right(r), is_leaf(false) {}

    size_t get_weight() const { 
        return is_leaf ? weight : left->get_weight() + right->get_weight(); 
    }
};


class htree {
    hnode * tree;
    ifstream & input;
    ofstream * output;
    bitset * code_map[MAX_VAL]; 

    char * out_fname;

public:
    htree(ifstream & inp): input(inp), output(nullptr), out_fname(nullptr) {
        memset(code_map, NULL, MAX_VAL);
    }

    void set_output(ofstream & out) {
        output = &out;
    }

    void set_output_name(const char * fname) {
        delete[] out_fname;
        out_fname = new char[strlen(fname)+1];
        strcpy(out_fname, fname);
    }

    ~htree() {
        _del_tree(tree);
        delete[] out_fname;
    }

    void compress() {
        if( !output )
            return;

        build_tree();

        input.seekg(0, ios::beg);
        output->seekp(0, ios::beg);
        
        size_t leafs = 0;
        output->write( (const char *) &leafs, sizeof(leafs));
        leafs = write_tree(tree, *output);

        size_t pos = output->tellp();
        output->seekp(0, ios::beg);
        output->write( (const char *) &leafs, sizeof(leafs));
        output->seekp(pos, ios::beg);

        size_t data_bits = 0;
        output->write( (const char *) &data_bits, sizeof(data_bits));
        pos = output->tellp();

        bitstream out(*output);
        data_t buff[CHUNK];

        while( input.read(buff, CHUNK) ) {
            for(size_t c = 0; c < input.gcount(); ++c) {
                bitset * code;
                code_of(buff[c], code);
                out << *code;
            }
        }

        data_bits = out.data_written() * 8 - out.flush();
        output->seekp(pos, ios::beg);
        output->write( (const char *) &data_bits, sizeof(data_bits));
    }

    void decompress() {
        if( !output )
            return;

        size_t leafs, data_bits;
        input.read( (char *) &leafs, sizeof(leafs));

        hnode ** nodes = new hnode*[leafs];
        for(size_t c = 0; c < leafs; ++c) {
            nodes[c] = new hnode;
            input.read( (char *) nodes[c], sizeof(nodes[c]->val) + sizeof(nodes[c]->weight));
        }
        build_tree(&tree, nodes, leafs);

    }

private:


    size_t write_tree(hnode * tree, ofstream & out) {
        if( tree->is_leaf ) {
            return !!out.write((const char*) tree, sizeof(tree->val) + sizeof(tree->weight));
        }
        return write_tree(tree->left, out) + write_tree(tree->right, out);
    }

    void code_of(data_t val, bitset * code) {
        if( code_map[val] ) {
            code_map[val] = new bitset(MAX_VAL);
        }
        code = code_map[val];
        code_of(val, *code, tree);
    }

    bool code_of(data_t val, bitset & code, hnode * tree, size_t lvl = 0) {
        if( tree->is_leaf ) {
            return val == tree->val;
        }

        if( code_of(val, code, tree->left, lvl + 1) ) {
            code.clear(lvl);
            return true;
        } else if( code_of(val, code, tree->right, lvl + 1) ) {
            code.set(lvl);
            return true;
        }

        return false;
    }

    void build_tree() {
        hnode ** list = new hnode*[MAX_VAL];
        memset(list, NULL, MAX_VAL);

        data_t buff[CHUNK];
        while( input.read(buff, CHUNK) ) {
            for(size_t c = 0; c < input.gcount(); ++c) {
                if( !list[buff[c]] ) {
                    list[buff[c]] = new hnode(buff[c]);
                }
                ++list[buff[c]]->weight;
            }
        }
        size_t size;
        compact_list(list, MAX_VAL, size);
        build_tree(&tree, list, size);
    }


    void build_tree(hnode ** root, hnode ** arr, size_t sz) {
        hnode * parent = nullptr;
        while(sz > 1) {
            sort_list(arr, sz);
            parent = new hnode(arr[0], arr[1]);
            arr[1] = arr[--sz];
            arr[0] = parent;
        }
        *root = arr[0];
    }
        
    void compact_list(hnode ** array, size_t len, size_t & new_len)
    {
        new_len = 0;
        for(size_t c = 0; c < len; ++c) 
            if( array[c] ) 
                array[new_len++] = array[c];
    }

    void sort_list(hnode ** list, size_t sz) {
        qsort(list, sz, sizeof(hnode *), [](const void * left, const void * right) -> int {
            hnode * _left  = (*(hnode**)left),
                  * _right = (*(hnode**)right);
            if( _left->get_weight() == _right->get_weight() ) {
                return !!(_left->val - _right->val);
            } else {
                return !!(_left->get_weight() - _right->get_weight());
            }
        });
    }

    void _del_tree(hnode * tree) {

    }
};

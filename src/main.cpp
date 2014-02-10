#include <string>
#include <cstdlib>
#include <algorithm>

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

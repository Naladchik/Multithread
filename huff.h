#ifndef HUFF_H
#define HUFF_J
#include <stdint.h>

/* Defines */
#define SYM_MAX 255
#define CODE_LEN_MAX 32 // bits
#define MAX_BUFFER 255000000 //max length of input text in symbols
#define H_OFFSET 5 // for header in zip file
#define S_COEFF 5 // for header in zip file

/* Types */
struct Node{
    uint32_t freq;
    char ch;
    bool leaf;
    struct Node * left;
    struct Node * right;
};

/* Function declarations */
void compress(char * source_file, char * output_file);   //main compression function
void uncompress(char * source_file, char * output_file); //main decompression function
uint32_t zip(uint8_t * bin_buff, char * text_buf, struct Node ** nodes_arr, uint32_t* lut_codes, uint8_t* lut_lengths, uint8_t nodes_len, uint32_t text_len);  //compression function (needs text buffer and codes to be prepared)
uint32_t unzip(uint8_t * bin_buff, char * text_buf, struct Node * hf_root); //decompression function (needs bytes buffer and Huffman tree to be prepared)

struct Node * CreateHuffTree(struct Node ** nodes_arr, uint8_t nodes_len);
void DeleteTree(struct Node * d_root);
void CreateCodes(struct Node * ptr, int depth, uint32_t* lut_codes, uint8_t* lengths); // depth argument is needed to implement recursion. Needs the Huffman tree to be ready
uint8_t MakeFrequencies(char * source_file, char * text_buf, struct Node ** nodes_arr, uint32_t* txt_len); // makes input array of nodes already sorted and ready for the Huffman tree building
struct Node * CreateEmptyNode(void); //utility function


#endif

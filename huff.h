#ifndef HUFF_H
#define HUFF_J
#include <stdint.h>

/* Function declarations */
void compress(char * source_file, char * output_file);   //main compression function
void uncompress(char * source_file, char * output_file); //main decompression function
uint32_t zip(uint8_t * bin_buff, char * text_buf);  //compression function (needs text buffer and codes to be prepared)
void unzip(uint8_t * bin_buff, char * text_buf); //decompression function (needs bytes buffer and Huffman tree to be prepared)

struct Node * CreateHuffTree();
void DeleteTree(struct Node * d_root);
void CreateCodes(struct Node * ptr, int depth); // depth argument is needed to implement recursion. Needs the Huffman tree to be ready
void MakeFrequencies(char * source_file, char * text_buf); // makes input array of nodes already sorted and ready for the Huffman tree building
struct Node * CreateEmptyNode(void); //utility function


#endif

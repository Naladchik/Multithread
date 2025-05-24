#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "huff.h"

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

/* Global variables */
struct Node * gv_arr_nodes[SYM_MAX] = {NULL};     //array of nodes for building the Huffman tree
uint8_t       gv_nodes_len = 0;                     //size of set of symbols to be coded in the Huffman tree

struct Node * gv_root = NULL;                     //pointer to the root of the Huffman tree when it's built

char          gv_text_big_buff[MAX_BUFFER] = {0}; //global text buffer for compression/decompression
uint32_t      gv_text_len = 0;                    //length of text stored in gv_text_big_buff in symbomls

uint32_t      gv_lut_codes[SYM_MAX] = {0};        //look up table to get code for compression
uint8_t       gv_lut_lengths[SYM_MAX] = {0};      //look up table to get length of code for compression

uint8_t       gv_bin_big_buff[MAX_BUFFER] = {0};  //global bytes buffer for compression/decompression
uint32_t      gv_bytes_len = 0;                   //number of bytes in the buffer including the header

bool gv_code_error = false;


/* Function definitions */
struct Node * CreateEmptyNode(void){
    struct Node * a_node = malloc(sizeof(struct Node));
    a_node->freq = 0;
    a_node->ch = 0;
    a_node->leaf = true;
    a_node->left = NULL;
    a_node->right = NULL;
    return(a_node);
}

/* Returns pointer to root */
struct Node * CreateHuffTree(){
    // Create the Huffman tree
    struct Node * arr_subtrees[gv_nodes_len];
    int16_t idx_leaves = gv_nodes_len - 1; // we start taking leaves from end of array (frequencies are in decreacing order)
    int16_t idx_subtrees = -1; // -1 means array is empty
    int16_t idx_to_put = 0; // -1 means array is empty
    struct  Node * node1 = NULL; // temporary node pointer
    struct  Node * node2 = NULL; // temporary node pointer
    struct  Node * temp_p = NULL; // temporary node pointer.
    uint8_t num_nodes = 0; // number of currently taken nodes
    bool finish = false;

    for(uint16_t i = 0; i < gv_nodes_len * gv_nodes_len + 10; i++){
        if(!finish){
            switch(num_nodes){
                case 0: // take first element                 
                    if((idx_leaves >= 0) && (idx_subtrees >= 0)){ //if both arrays are not empty
                        // choose between a leaf and a subtree
                        if(arr_subtrees[idx_subtrees]->freq < gv_arr_nodes[idx_leaves]->freq){
                            node1 = arr_subtrees[idx_subtrees];
                            idx_subtrees--;
                            num_nodes++;
                        }else{
                            node1 = gv_arr_nodes[idx_leaves];
                            idx_leaves--;
                            num_nodes++;
                        }
                    }else{
                        if(idx_leaves >= 0){ // have only leaves available
                            node1 = gv_arr_nodes[idx_leaves];
                            idx_leaves--;
                            num_nodes++;
                            if(0 > idx_leaves){ // scenario when only 1 leaf
                                temp_p = CreateEmptyNode();
                                temp_p->freq = node1->freq;
                                temp_p->ch = 0;
                                temp_p->leaf = false;
                                temp_p->left = node1;
                                temp_p->right = NULL;
                                node1 = temp_p;
                                node2 = NULL;
                                finish = true;
                            }
                        }else{
                            if(idx_subtrees >= 0){ // have only a subtree available
                                node1 = arr_subtrees[idx_subtrees];
                                idx_subtrees--;
                                num_nodes++;
                            }else{// have no leaves and no subtrees
                                node1 = NULL;
                                finish = true;
                            }
                        }
                    }
                    break;
                case 1: // take second element
                    if((idx_leaves >= 0) && (idx_subtrees >= 0)){
                        // choose between a leaf and a subtree
                        if(arr_subtrees[idx_subtrees]->freq < gv_arr_nodes[idx_leaves]->freq){
                            node2 = arr_subtrees[idx_subtrees];
                            idx_subtrees--;
                            num_nodes++;
                        }else{
                            node2 = gv_arr_nodes[idx_leaves];
                            idx_leaves--;
                            num_nodes++;
                        }
                    }else{
                        if(idx_leaves >= 0){ // have only a leaf available
                            node2 = gv_arr_nodes[idx_leaves];
                            idx_leaves--;
                            num_nodes++;
                        }else{
                            if(idx_subtrees >= 0){ // have only subtrees available
                                node2 = arr_subtrees[idx_subtrees];
                                idx_subtrees--;
                                num_nodes++;
                            }else{// have no leaves and no subtrees
                                finish = true;
                            }
                        }
                    }
                    break;
                case 2: // create element from first and second
                    num_nodes = 0;
                    //create a new subtree
                    temp_p = CreateEmptyNode();
                    temp_p->freq = node1->freq + node2->freq;
                    temp_p->ch = 0;
                    temp_p->leaf = false;
                    temp_p->left = node1;
                    temp_p->right = node2;
                    node1 = NULL;
                    node2 = NULL;

                    //and put it to array of subtrees
                    idx_to_put = 0;
                    for(int16_t i = idx_subtrees; i >=0; i--){ // find the right place
                        if(temp_p->freq < arr_subtrees[i]->freq){
                            idx_to_put = i;
                            break;
                        }
                    }
                    for(int16_t i = idx_subtrees; i >= idx_to_put; i--){ // move elements
                        arr_subtrees[i + 1] = arr_subtrees[i];
                    }
                    arr_subtrees[idx_to_put] = temp_p;                    
                    idx_subtrees++;
                    break;

                default:
                    printf("Error! num_nodes is more than 2!"); 
                    break;
            }
        }else{
            break;
        }
        
    }
    return(node1);
}

// Creates huffman codes from the root of Huffman Tree.
// It's a recursion
void CreateCodes(struct Node* ptr, int depth){
    static uint16_t idx = 0; // current number of resolved symbols (leaves aproached)
    static uint8_t arr01[CODE_LEN_MAX]; // buffer-array for storing 0 and 1 of code    
    // Assign 0 to left edge and recur
    if (ptr->left) { 
        arr01[depth] = 0;
        CreateCodes(ptr->left, depth + 1);
    }
 
    // Assign 1 to right edge and recur
    if (ptr->right) {
        arr01[depth] = 1;
        CreateCodes(ptr->right, depth + 1);
    }
 
    if (ptr->leaf) {
        gv_lut_lengths[ptr->ch] = depth;
        for(char i = 0; i < depth; i++){
            gv_lut_codes[ptr->ch] <<= 1;
            if(arr01[i]){
                gv_lut_codes[ptr->ch] |= 0x00000001;
            }
        }
        idx++;
    }
}

uint32_t zip(void){
    // bytes 0 - 3 contain size of input text (less number - less significat byte)
    // byte 4 contains number of coded symbols
    // byte4 * 5 bytes contain symbol and four bytes of frequency for this symbol
    // after that coding itself goes

    //Creates the bunch of data in the beginning of file needed for future decompression.    
    uint32_t out_idx = 0;
    uint32_t out_idx_bit = 0;
    uint32_t mask = 0;
    uint8_t shift = 0;

    *((uint32_t *) (&(gv_bin_big_buff[0]))) = gv_text_len; // 4 bytes 0, 1, 2 and 3

    //table part
    gv_bin_big_buff[4] = gv_nodes_len;
    out_idx = H_OFFSET;
    for(uint8_t i = 0; i < gv_nodes_len; i++){
        gv_bin_big_buff[out_idx] = gv_arr_nodes[i]->ch;        
        out_idx++;
        *((uint32_t *) (&(gv_bin_big_buff[out_idx]))) = gv_arr_nodes[i]->freq;
        out_idx += 4;
    }

    //coding part
    out_idx_bit = out_idx * 8;
    for(int i = 0; i < gv_text_len; i++){
        mask = gv_lut_codes[gv_text_big_buff[i]];
        for(char j = (gv_lut_lengths[gv_text_big_buff[i]] - 1); j >=0; j--){
            out_idx = out_idx_bit / 8;
            shift = out_idx_bit % 8;
            if(mask & (1<<j)){
                gv_bin_big_buff[out_idx] |= (0x80 >> (shift));
            }
            out_idx_bit++;
        }       
    }

    gv_bytes_len = out_idx + 1;
    return(gv_bytes_len);
}

void unzip(void){
    // bytes 0 - 3 contain size of input text (less number - less significat byte)
    // byte 4 contains number of coded symbols
    // byte4 * 5 bytes contain symbol and four bytes of frequency for this symbol
    // after that coding itself goes

    struct Node * ptr = gv_root;
    uint32_t start_idx;
    uint32_t text_counter = 0;
 
    start_idx = H_OFFSET + gv_bin_big_buff[4] * S_COEFF; // calculate starting byte
    start_idx *= 8; // convert it into starting bit

    // extract length of text
    gv_text_len = *((uint32_t *) (&(gv_bin_big_buff[0])));

    for(uint32_t i = start_idx; i < 4000000000; i++){ // go bit by bit from MSB to LSB
        if(text_counter == gv_text_len) break;
        if(gv_bin_big_buff[i/8] & (0x80 >> (i%8))){ //1            
            if(!ptr->leaf){ ptr->leaf;
                ptr = ptr->right;
                if(ptr->leaf){
                    gv_text_big_buff[text_counter] = ptr->ch;
                    text_counter++;
                    ptr = gv_root;
                }
            }
        }else{                                      //0
            if(!ptr->leaf){
                ptr = ptr->left;
                if(ptr->leaf){
                    gv_text_big_buff[text_counter] = ptr->ch;
                    text_counter++;
                    ptr = gv_root;
                }
            }
        }
    }
}

void MakeFrequencies(char * source_file){
    char ch;
    uint32_t in_idx = 0;
    uint32_t lut_frequencies[SYM_MAX] = {0};
    FILE * in_f = fopen(source_file, "r");

    if(NULL == in_f){
        printf("file %s couldn't be opened (MakeFrequencies()).\n", source_file);
    }else{
        do{
            ch = fgetc(in_f);
            if(-1 > ch){ // -1 because EOF is accepted
                printf("wrong symbol %c, code %d\n", ch, ch);
                gv_code_error = true;
            }else{
                gv_text_big_buff[in_idx] = ch;
                in_idx++;
                if(0 < ch){lut_frequencies[ch]++;};
            }
        }while(ch != EOF);
        fclose(in_f);
        gv_text_len = in_idx - 1;
    }

    //making a sorted (by frequencies in decreasing order) array of nodes
    gv_nodes_len = 0;
    uint32_t max_f;
    uint8_t max_j;
    for(uint8_t i = 0; i <= 127; i++){
        max_f = 0;
        for(uint8_t j = 0; j <= 127; j++){
            if(lut_frequencies[j] > max_f){
                 max_f = lut_frequencies[j];
                 max_j = j;
            }
        }
        if(0 == max_f){
            break;
        }else{
            gv_arr_nodes[gv_nodes_len] = CreateEmptyNode();
            gv_arr_nodes[gv_nodes_len]->ch = max_j;
            gv_arr_nodes[gv_nodes_len]->leaf = true;
            gv_arr_nodes[gv_nodes_len]->freq = lut_frequencies[max_j];
            gv_nodes_len++; //increment the global variable
            lut_frequencies[max_j] = 0;
        }
    }
}

void compress(char * source_file, char * output_file){
    __label__ FINISH;
    MakeFrequencies(source_file);
    if(gv_code_error){ printf(" ERROR!!! Unaccepted symbol code"); goto FINISH; }
    gv_root = CreateHuffTree();

    if(gv_root){
        CreateCodes(gv_root, 0);
    }else{
        printf("Something went wrong during Huffman tree creatiion\n");
    }

    zip();
    DeleteTree(gv_root);

    FILE *write_ptr;
    write_ptr = fopen(output_file,"wb");  // w for write, b for binary
    fwrite(gv_bin_big_buff, gv_bytes_len, 1, write_ptr);
    fclose(write_ptr);
    FINISH:;
}

void uncompress(char * source_file, char * output_file){
    uint8_t rd_buf;

    //read binary from input file
    FILE *f_ptr;
    f_ptr = fopen(source_file,"rb");  // r for read, b for binary

    if(NULL == f_ptr){
        printf("file %s couldn't be opened (uncompress()).\n", source_file);
    }else{
        gv_bytes_len = 0;
        for(uint32_t i = 0; i < MAX_BUFFER; i++){
            if(fread(&rd_buf, 1, 1 ,f_ptr)){
                gv_bin_big_buff[i] = rd_buf;
            }else{
                gv_bytes_len = i - 1;
                break;
            }        
        }
        fclose(f_ptr);

        //restore the Huffman tree
        uint32_t fr;
        gv_nodes_len = gv_bin_big_buff[4];
        //printf("gv_nodes_len = %d\n", gv_nodes_len);
        for(uint8_t i = 0; i < gv_nodes_len; i++){
            gv_arr_nodes[i] = CreateEmptyNode();
            gv_arr_nodes[i]->ch = gv_bin_big_buff[H_OFFSET + i * S_COEFF];
            gv_arr_nodes[i]->leaf = true;
            fr = *((uint32_t *) (&(gv_bin_big_buff[H_OFFSET + i * S_COEFF + 1])));
            gv_arr_nodes[i]->freq = fr;
        }
        gv_root = CreateHuffTree();

        //decode text
        unzip();

        DeleteTree(gv_root);

        //write text to the file
        FILE *write_ptr;
        write_ptr = fopen(output_file,"w");  // w for write
        fwrite(gv_text_big_buff, gv_text_len, 1, write_ptr); // write 
        fclose(write_ptr);
    }    
}

void DeleteTree(struct Node * d_root){
    if (d_root == NULL) {
        return;
    }    
    DeleteTree(d_root->left);
    DeleteTree(d_root->right); 
    free(d_root); 
    d_root = NULL;
}

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "huff.h"

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
struct Node * CreateHuffTree(struct Node ** nodes_arr, uint8_t nodes_len){
    // Create the Huffman tree
    struct Node * arr_subtrees[nodes_len];
    int16_t idx_leaves = nodes_len - 1; // we start taking leaves from end of array (frequencies are in decreacing order)
    int16_t idx_subtrees = -1; // -1 means array is empty
    int16_t idx_to_put = 0; // -1 means array is empty
    struct  Node * node1 = NULL; // temporary node pointer
    struct  Node * node2 = NULL; // temporary node pointer
    struct  Node * temp_p = NULL; // temporary node pointer.
    uint8_t num_nodes = 0; // number of currently taken nodes
    bool finish = false;

    for(uint16_t i = 0; i < nodes_len * nodes_len + 10; i++){
        if(!finish){
            switch(num_nodes){
                case 0: // take first element                 
                    if((idx_leaves >= 0) && (idx_subtrees >= 0)){ //if both arrays are not empty
                        // choose between a leaf and a subtree
                        if(arr_subtrees[idx_subtrees]->freq < nodes_arr[idx_leaves]->freq){
                            node1 = arr_subtrees[idx_subtrees];
                            idx_subtrees--;
                            num_nodes++;
                        }else{
                            node1 = nodes_arr[idx_leaves];
                            idx_leaves--;
                            num_nodes++;
                        }
                    }else{
                        if(idx_leaves >= 0){ // have only leaves available
                            node1 = nodes_arr[idx_leaves];
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
                        if(arr_subtrees[idx_subtrees]->freq < nodes_arr[idx_leaves]->freq){
                            node2 = arr_subtrees[idx_subtrees];
                            idx_subtrees--;
                            num_nodes++;
                        }else{
                            node2 = nodes_arr[idx_leaves];
                            idx_leaves--;
                            num_nodes++;
                        }
                    }else{
                        if(idx_leaves >= 0){ // have only a leaf available
                            node2 = nodes_arr[idx_leaves];
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
void CreateCodes(struct Node* ptr, int depth, uint32_t* lut_codes, uint8_t* lut_lengths){
    static uint16_t idx = 0; // current number of resolved symbols (leaves aproached)
    static uint8_t arr01[CODE_LEN_MAX]; // buffer-array for storing 0 and 1 of code    
    // Assign 0 to left edge and recur
    if (ptr->left) { 
        arr01[depth] = 0;
        CreateCodes(ptr->left, depth + 1, lut_codes, lut_lengths);
    }
 
    // Assign 1 to right edge and recur
    if (ptr->right) {
        arr01[depth] = 1;
        CreateCodes(ptr->right, depth + 1, lut_codes, lut_lengths);
    }
 
    if (ptr->leaf) {
        lut_lengths[ptr->ch] = depth;
        for(char i = 0; i < depth; i++){
        	lut_codes[ptr->ch] <<= 1;
            if(arr01[i]){
            	lut_codes[ptr->ch] |= 0x00000001;
            }
        }
        idx++;
    }
}

uint32_t zip(uint8_t * bin_buff, char * text_buf, struct Node ** nodes_arr, uint32_t* lut_codes, uint8_t* lut_lengths, uint8_t nodes_len, uint32_t text_len){
    // bytes 0 - 3 contain size of input text (less number - less significat byte)
    // byte 4 contains number of coded symbols
    // byte4 * 5 bytes contain symbol and four bytes of frequency for this symbol
    // after that coding itself goes

    //Creates the bunch of data in the beginning of file needed for future decompression.    
    uint32_t out_idx = 0;
    uint32_t out_idx_bit = 0;
    uint32_t mask = 0;
    uint8_t shift = 0;

    *((uint32_t *) (&(bin_buff[0]))) = text_len; // 4 bytes 0, 1, 2 and 3

    //table part
    bin_buff[4] = nodes_len;
    out_idx = H_OFFSET;
    for(uint8_t i = 0; i < nodes_len; i++){
    	bin_buff[out_idx] = nodes_arr[i]->ch;
        out_idx++;
        *((uint32_t *) (&(bin_buff[out_idx]))) = nodes_arr[i]->freq;
        out_idx += 4;
    }

    //coding part
    out_idx_bit = out_idx * 8;
    for(int i = 0; i < text_len; i++){
        mask = lut_codes[text_buf[i]];
        for(char j = (lut_lengths[text_buf[i]] - 1); j >=0; j--){
            out_idx = out_idx_bit / 8;
            shift = out_idx_bit % 8;
            if(mask & (1<<j)){
            	bin_buff[out_idx] |= (0x80 >> (shift));
            }
            out_idx_bit++;
        }       
    }

    return(out_idx + 1);
}

uint32_t unzip(uint8_t * bin_buff, char * text_buf, struct Node * hf_root){
    // bytes 0 - 3 contain size of input text (less number - less significat byte)
    // byte 4 contains number of coded symbols
    // byte4 * 5 bytes contain symbol and four bytes of frequency for this symbol
    // after that coding itself goes

    struct Node * ptr = hf_root;
    uint32_t start_idx;
    uint32_t text_counter = 0;
 
    start_idx = H_OFFSET + bin_buff[4] * S_COEFF; // calculate starting byte
    start_idx *= 8; // convert it into starting bit

    // extract length of text
    uint32_t read_text_len = *((uint32_t *) (&(bin_buff[0])));

    for(uint32_t i = start_idx; i < 4000000000; i++){ // go bit by bit from MSB to LSB
        if(text_counter == read_text_len) break;
        if(bin_buff[i/8] & (0x80 >> (i%8))){ //1
            if(!ptr->leaf){ ptr->leaf;
                ptr = ptr->right;
                if(ptr->leaf){
                	text_buf[text_counter] = ptr->ch;
                    text_counter++;
                    ptr = hf_root;
                }
            }
        }else{                                      //0
            if(!ptr->leaf){
                ptr = ptr->left;
                if(ptr->leaf){
                	text_buf[text_counter] = ptr->ch;
                    text_counter++;
                    ptr = hf_root;
                }
            }
        }
    }
    return(read_text_len);
}

uint8_t MakeFrequencies(char * source_file, char * text_buf, struct Node ** nodes_arr, uint32_t* txt_len, bool* gv_code_error){
    char ch;
    uint32_t in_idx = 0;
    uint32_t lut_frequencies[SYM_MAX] = {0};
    uint8_t       gv_nodes_len = 0;                     //size of set of symbols to be coded in the Huffman tree
    FILE * in_f = fopen(source_file, "r");

    if(NULL == in_f){
        printf("file %s couldn't be opened (MakeFrequencies()).\n", source_file);
    }else{
        do{
            ch = fgetc(in_f);
            if(-1 > ch){ // -1 because EOF is accepted
                printf("wrong symbol %c, code %d\n", ch, ch);
                *gv_code_error = true;
            }else{
            	text_buf[in_idx] = ch;
                in_idx++;
                if(0 < ch){lut_frequencies[ch]++;};
            }
        }while(ch != EOF);
        fclose(in_f);
        *txt_len = in_idx - 1;
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
        	nodes_arr[gv_nodes_len] = CreateEmptyNode();
        	nodes_arr[gv_nodes_len]->ch = max_j;
        	nodes_arr[gv_nodes_len]->leaf = true;
        	nodes_arr[gv_nodes_len]->freq = lut_frequencies[max_j];
            gv_nodes_len++; //increment the global variable
            lut_frequencies[max_j] = 0;
        }
    }
    return(gv_nodes_len);
}

void compress(char * source_file, char * output_file){
    __label__ FINISH;
    uint8_t * gv_bin_big_buff = (uint8_t *)malloc(sizeof(uint8_t) * MAX_BUFFER);
    char * gv_text_big_buff = (char *)calloc(MAX_BUFFER, sizeof(char));
    struct Node * gv_arr_nodes[SYM_MAX] = {NULL};     //array of nodes for building the Huffman tree
    uint32_t      gv_lut_codes[SYM_MAX] = {0};        //look up table to get code for compression
    uint8_t       gv_lut_lengths[SYM_MAX] = {0};      //look up table to get length of code for compression
    uint8_t       gv_nodes_len;                     //size of set of symbols to be coded in the Huffman tree
    struct Node * gv_root = NULL;
    uint32_t      gv_text_len = 0;                    //length of text stored in gv_text_big_buff in symbomls
    uint32_t      cm_bytes_len = 0;                   //number of bytes in the buffer including the header
    bool gv_code_error = false;

    gv_nodes_len = MakeFrequencies(source_file, gv_text_big_buff, gv_arr_nodes, &gv_text_len, &gv_code_error);
    if(gv_code_error){ printf(" ERROR!!! Unaccepted symbol code"); goto FINISH; }
    gv_root = CreateHuffTree(gv_arr_nodes, gv_nodes_len);

    if(gv_root){
        CreateCodes(gv_root, 0, gv_lut_codes, gv_lut_lengths);
    }else{
        printf("Something went wrong during Huffman tree creatiion\n");
    }

    cm_bytes_len = zip(gv_bin_big_buff, gv_text_big_buff, gv_arr_nodes, gv_lut_codes, gv_lut_lengths, gv_nodes_len, gv_text_len);
    DeleteTree(gv_root);

    FILE *write_ptr;
    write_ptr = fopen(output_file,"wb");  // w for write, b for binary
    fwrite(gv_bin_big_buff, cm_bytes_len, 1, write_ptr);
    fclose(write_ptr);
    free(gv_bin_big_buff);
    free(gv_text_big_buff);
    FINISH:;
}

void uncompress(char * source_file, char * output_file){
    uint8_t rd_buf;
    uint8_t * gv_bin_big_buff = malloc(sizeof(uint8_t) * MAX_BUFFER);
    char * gv_text_big_buff = (char *)calloc(MAX_BUFFER, sizeof(char));
    struct Node * gv_arr_nodes[SYM_MAX] = {NULL};     //array of nodes for building the Huffman tree
    uint8_t       gv_nodes_len = 0;                     //size of set of symbols to be coded in the Huffman tree
    struct Node * gv_root = NULL;                     //pointer to the root of the Huffman tree
    uint32_t      unc_bytes_len = 0;                   //number of bytes in the buffer including the header

    //read binary from input file
    FILE *f_ptr;
    f_ptr = fopen(source_file,"rb");  // r for read, b for binary

    if(NULL == f_ptr){
        printf("file %s couldn't be opened (uncompress()).\n", source_file);
    }else{
    	unc_bytes_len = 0;
        for(uint32_t i = 0; i < MAX_BUFFER; i++){
            if(fread(&rd_buf, 1, 1 ,f_ptr)){
                gv_bin_big_buff[i] = rd_buf;
            }else{
                unc_bytes_len = i - 1;
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
        gv_root = CreateHuffTree(gv_arr_nodes, gv_nodes_len);

        //decode text
        uint32_t text_len = unzip(gv_bin_big_buff, gv_text_big_buff, gv_root);

        DeleteTree(gv_root);

        //write text to the file
        FILE *write_ptr;
        write_ptr = fopen(output_file,"w");  // w for write
        fwrite(gv_text_big_buff, text_len, 1, write_ptr); // write
        fclose(write_ptr);

    }
    free(gv_bin_big_buff);
    free(gv_text_big_buff);
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

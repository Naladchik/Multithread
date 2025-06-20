#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>
#include "huff.h"

#define IN_FILE "to_compress/text2"
#define OUT_FILE "compressed/text2.zip"
#define VALIDATION_FILE "to_compress/text2_val"

float prgs;

int main() {
	compress(IN_FILE, OUT_FILE, &prgs);
	uncompress(OUT_FILE, VALIDATION_FILE, &prgs);
    return 0;
}

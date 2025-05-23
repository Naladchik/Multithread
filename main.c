#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>
#include "huff.h"

#define MAX_IN_PROGRESS 100
#define IN_DIR "to_compress"
#define OUT_DIR "compressed"

char str_arr[100][100];

struct MondStruct{ //Monitored tasks. One variable per task passed as argument.
	char* in_file;
	char* out_file;
	int num;
};

struct MonStruct{
	int task_num; //number of monitored tasks
	float progress[MAX_IN_PROGRESS]; //array of tasks status from 0.0 to 100.0 %
};

/* SHARED VARIABLES */
struct MonStruct mon_arg;  // One variable (containing array of floats) for all tasks. Implicitly updated from tasks.
pthread_mutex_t mon_arg_lock;

void* monitor_thr(void* m_arg){
	do
	{
		bool any_in_progress = false;
		for(int i = 0; i < ((struct MonStruct *)m_arg)->task_num; i++){
			printf("[task %d]: %.1f%%\n", i, ((struct MonStruct *)m_arg)->progress[i]);
			((struct MonStruct *)m_arg)->progress[i] += 0.01;
			if(((struct MonStruct *)m_arg)->progress[i] < 100.0) any_in_progress = true;
		}
		sleep(0.1);
		if(!any_in_progress) break;
		//clear_screen(100, ((struct MonStruct *)m_arg)->task_num);
		printf("\n\n");
	}
	while(1);
	return NULL;
}

void* monitored_thr(void* md_arg){
//	printf("task %d started, input: [%s], output: [%s]\n",
//			((struct MondStruct*)md_arg)->num,
//			((struct MondStruct*)md_arg)->in_file,
//			((struct MondStruct*)md_arg)->out_file);

//	do
//	{
//		pthread_mutex_lock(&mon_arg_lock);
//		mon_arg.progress[((struct MondStruct*)md_arg)->num] += 2.0;
//		pthread_mutex_unlock(&mon_arg_lock);
//		if(mon_arg.progress[((struct MondStruct*)md_arg)->num] >= 100.1) break;
//		sleep(0.15);
//	}
//	while(1);
	compress(((struct MondStruct*)md_arg)->in_file, ((struct MondStruct*)md_arg)->out_file);
	mon_arg.progress[((struct MondStruct*)md_arg)->num] = 100.0;
	printf("task completed\n");
	return NULL;
}

int main() {
	printf("PROGRAM STARTED\n\n");
	// global variable initialization
	for(int i = 0; i < MAX_IN_PROGRESS; i++){
		mon_arg.progress[i] = 0.0;
	}

	pthread_t thread_main;

	//files operations
	DIR *d;
	char* f_str;
	int file_counter = 0;
	struct dirent *dir;
	d = opendir(IN_DIR);
	if (d) {
		printf("directory [%s] opened\n", IN_DIR);
		while ((dir = readdir(d)) != NULL) {
			f_str = dir->d_name;
			if(f_str[0] != '.'){
				printf("[%s]\n", f_str);
				strcpy(str_arr[file_counter], f_str);
				file_counter++;
			}
	    }
	    closedir(d);
	    printf("there are [%d] files to compress, let's go:\n\n", file_counter);
	}else{
		printf("Could not open directory: %s", IN_DIR);
	}
	sleep(2);

	mon_arg.task_num = file_counter;

    pthread_t* thr_arr = (pthread_t*)malloc(sizeof(pthread_t) * mon_arg.task_num);
    struct MondStruct* thr_arg_arr = (struct MondStruct*)malloc(sizeof(struct MondStruct) * mon_arg.task_num);
    for(int i = 0; i < mon_arg.task_num; i++){
    	thr_arg_arr[i].num = i;
    	char str_buf[100];
    	strcpy(str_buf, IN_DIR);
    	strcat(str_buf, "/");
    	strcat(str_buf, str_arr[i]);
    	thr_arg_arr[i].in_file = str_buf;
    	strcpy(str_buf, OUT_DIR);
    	strcat(str_buf, "/");
    	strcat(str_buf, str_arr[i]);
    	strcat(str_buf, ".xxx");
    	thr_arg_arr[i].out_file = str_buf;
    }

    pthread_create(&thread_main, NULL, monitor_thr, &mon_arg);
    for(int i = 0; i < mon_arg.task_num; i++){
    	pthread_create(&(thr_arr[i]), NULL, monitored_thr, &(thr_arg_arr[i]));
    	sleep(0.3);
    }

    pthread_join(thread_main, NULL);
    for(int i = 0; i < mon_arg.task_num; i++){
    	pthread_join(thr_arr[i], NULL);
    }

    free(thr_arr);
    free(thr_arg_arr);
    printf("\nPROGRAM FINISHED\n");
    return 0;
}

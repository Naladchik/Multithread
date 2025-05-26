#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <dirent.h>
#include <ncurses.h>
#include "huff.h"

#define MAX_LEN 200
#define MAX_IN_PROGRESS 100
#define C_DIR "to_compress" //C means 'directory containing file to COMPRESS
#define U_DIR "compressed"  //U means 'directory containing file to UNCOMPRESS

//#define VALIDATION_FILE "to_compress/text1_val"

char str_arr_c[MAX_IN_PROGRESS][MAX_LEN];  //for handling files paths before multitasking
char str_arr_u[MAX_IN_PROGRESS][MAX_LEN];  //for handling files paths before multitasking

struct MondStruct{ //Monitored tasks. One variable per task passed as argument.
	char in_file[MAX_LEN];
	char out_file[MAX_LEN];
	int num;
};

struct MonStruct{
	int task_num; //number of monitored tasks
	float progress[MAX_IN_PROGRESS]; //array of tasks status from 0.0 to 100.0 %
};

/* SHARED VARIABLES */
struct MonStruct mon_arg;  // One variable (containing array of floats) for all tasks. Implicitly updated from tasks.
pthread_mutex_t mon_arg_lock;
pthread_cond_t cond;
char cu_var;  //not used in multi threading

void* monitor_thr(void* m_arg){
	char str_buf[100] = {'\0'};
	initscr();
	cbreak();
	noecho();

	clear();

	do
	{
//		bool any_in_progress = false;
//		for(int i = 0; i < ((struct MonStruct *)m_arg)->task_num; i++){
//			if(((struct MonStruct *)m_arg)->progress[i] < 99.99){
//				any_in_progress = true;
//				printf("[task %d]: %.1f%%\n", i, ((struct MonStruct *)m_arg)->progress[i]);
//			}else{
//				printf("[task %d]: Done\n", i);
//			}
//		}
//		if(!any_in_progress) break;
//		printf("\n\n");

		bool any_in_progress = false;
		for(int i = 0; i < ((struct MonStruct *)m_arg)->task_num; i++){
			if(((struct MonStruct *)m_arg)->progress[i] < 99.99){
				any_in_progress = true;
				sprintf(str_buf, "[task %d]: %.1f%%                   \n", i, ((struct MonStruct *)m_arg)->progress[i]);
			}else{
				sprintf(str_buf, "[task %d]: Done                     \n", i);
			}
			mvaddstr(i, 0, str_buf);
		}
		if(!any_in_progress) break;

		refresh();

		pthread_mutex_lock(&mon_arg_lock);
		pthread_cond_wait(&cond, &mon_arg_lock);
		pthread_mutex_unlock(&mon_arg_lock);
	}
	while(1);

	mvaddstr(((struct MonStruct *)m_arg)->task_num, 0, "PROGRAM FINISHED. PRESS ANY KEY.\n");
	refresh();

	getch();
	endwin();

	return NULL;
}

void* monitored_thr(void* md_arg){
	if(cu_var == 'c')   compress(((struct MondStruct*)md_arg)->in_file, ((struct MondStruct*)md_arg)->out_file, &(mon_arg.progress[((struct MondStruct*)md_arg)->num]));
	if(cu_var == 'u') uncompress(((struct MondStruct*)md_arg)->in_file, ((struct MondStruct*)md_arg)->out_file, &(mon_arg.progress[((struct MondStruct*)md_arg)->num]));
	return NULL;
}

int main() {
	printf("PROGRAM STARTED\n\n");
	// global variable initialization
	for(int i = 0; i < MAX_IN_PROGRESS; i++){
		mon_arg.progress[i] = 0.0;
	}

	if (pthread_mutex_init(&mon_arg_lock, NULL) != 0) {
		    perror("mutex_lock");
		    exit(1);
		}

	pthread_t thread_main;

	//files operations
	DIR *d;
	char* f_str;
	int file_c_counter = 0;
	int file_u_counter = 0;
	struct dirent *dir;
	d = opendir(C_DIR);
	if (d) {
		printf("directory [%s] opened\n", C_DIR);
		while ((dir = readdir(d)) != NULL) {
			f_str = dir->d_name;
			if(f_str[0] != '.'){
				printf("[%s]\n", f_str);
				strcpy(str_arr_c[file_c_counter], f_str);
				file_c_counter++;
			}
	    }
	    closedir(d);
	    printf("there are [%d] files to compress\n\n", file_c_counter);
	}else{
		printf("Could not open directory: %s", C_DIR);
	}

	d = opendir(U_DIR);
	if (d) {
			printf("directory [%s] opened\n", U_DIR);
			while ((dir = readdir(d)) != NULL) {
				f_str = dir->d_name;
				if(f_str[0] != '.'){
					printf("[%s]\n", f_str);
					strcpy(str_arr_u[file_u_counter], f_str);
					file_u_counter++;
				}
		    }
		    closedir(d);
		    printf("there are [%d] files to uncompress\n\n", file_u_counter);
		}else{
			printf("Could not open directory: %s", U_DIR);
		}

	do{
	    printf("Do you want to compress or uncompress) [c/u]: ");
	    cu_var = getchar();
	    printf("\n");
	}while((cu_var != 'c') && (cu_var != 'u'));

	if(cu_var == 'c')
		mon_arg.task_num = file_c_counter;
	else if(cu_var == 'u')
		mon_arg.task_num = file_u_counter;

	char* c_dir = C_DIR;
	char* u_dir = U_DIR;
	char* in_dir;
	char* out_dir;

	if(cu_var == 'c'){
		in_dir = c_dir;
		out_dir = u_dir;
	}else if(cu_var == 'u'){
		in_dir = u_dir;
		out_dir = c_dir;
	}

	printf("in_dir: %s\n", in_dir);
	printf("out_dir: %s\n", out_dir);

	/*
	 THREADS CREATED HERE
	 */

    pthread_t* thr_arr = (pthread_t*)malloc(sizeof(pthread_t) * mon_arg.task_num);
    struct MondStruct* thr_arg_arr = (struct MondStruct*)malloc(sizeof(struct MondStruct) * mon_arg.task_num);
    for(int i = 0; i < mon_arg.task_num; i++){
    	thr_arg_arr[i].num = i;
    	char str_buf[200];
    	if(cu_var == 'c'){
    		strcpy(str_buf, in_dir);
    		strcat(str_buf, "/");
    		strcat(str_buf, str_arr_c[i]);
    		strcpy(thr_arg_arr[i].in_file, str_buf);
    		strcpy(str_buf, out_dir);
    		strcat(str_buf, "/");
    		strcat(str_buf, str_arr_c[i]);
    		strcat(str_buf, ".zip");
    		strcpy(thr_arg_arr[i].out_file, str_buf);
    	}else{
    		strcpy(str_buf, in_dir);
    		strcat(str_buf, "/");
       		strcat(str_buf, str_arr_u[i]);
       		strcpy(thr_arg_arr[i].in_file, str_buf);
       		strcpy(str_buf, out_dir);
       		strcat(str_buf, "/");
       		strcat(str_buf, str_arr_u[i]);
       		strcat(str_buf, "_decompressed");
       		strcpy(thr_arg_arr[i].out_file, str_buf);
    	}
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

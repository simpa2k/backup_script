//
//  main.c
//  backup_script
//
//  Created by Simon Olofsson on 2016-09-05.
//  Copyright © 2016 Simon Olofsson. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <copyfile.h>
#include <stdbool.h>

#import <pthread.h>

#include "stack.h"

#define CPUS 2
#define NUM_THREADS CPUS + 1

Stack *populate_stack();
int threaded_backup(Stack *S, char *destination);
void *copydir_thread(void *thread_args);
int copy_file(const char *from, const char *to);
int copydir(const char *dirname, const char *destination_root);
int compare_dates(const char *file1, const char *file2);
char *get_filename(const char *path);
//void concat_paths(char *output, long output_size, const char *base, const char *extension);
char *concat_paths(const char *base, const char *extension);

struct thread_args {
    Stack *S;
    char destination[1024];
    pthread_mutex_t lock;
};

int main(int argc, const char * argv[]) {
    
    Stack *S = populate_stack();
    char destination[] = "/Users/simpa2k/C/backup_script/backup_script/destination";
    
    return threaded_backup(S, destination);
}

Stack *populate_stack() {
    char *paths[3];
    paths[0] = "/Users/simpa2k/C/backup_script/backup_script/testdir";
    paths[1] = "/Users/simpa2k/C/backup_script/backup_script/testdir2";
    paths[2] = "/Users/simpa2k/C/backup_script/backup_script/testdir3";
    
    Stack *S = createStack();
    
    for(int i = 0; i < sizeof(paths) / sizeof(char *); i++) {
        push(S, paths[i]);
    }
    
    return S;
}

int threaded_backup(Stack *S, char *destination) {
    pthread_t threads[2];
    pthread_mutex_t lock;
    
    if(pthread_mutex_init(&lock, NULL) != 0) {
        printf("Mutex init failed\n");
        return -1;
    };
    
    for(int i = 0; i < NUM_THREADS && i < getSize(S); i++) {
        struct thread_args *thr_args = malloc(sizeof(*thr_args));
        
        thr_args->S = S;
        strcpy(thr_args->destination, destination);
        thr_args->lock = lock;
        
        pthread_create(&threads[i], NULL, copydir_thread, thr_args);
        
    }
    
    for(int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    pthread_mutex_destroy(&lock);

    return 0;
}

void *copydir_thread(void *thread_args) {
    struct thread_args *thr_args = thread_args;
    
    while(true) {
        pthread_mutex_lock(&thr_args->lock);
        char *path = pop(thr_args->S);
        pthread_mutex_unlock(&thr_args->lock);
        
        if(path != NULL) {
            copydir(path, thr_args->destination);
        } else {
            break;
        }
        
    }
    
    
    return NULL;
}

int copydir(const char *dirname, const char *destination_root) {
    struct dirent *pDirent;
    struct stat st;
    DIR *pDir;
    
    pDir = opendir(dirname);
    
    if(pDir == NULL) {
        
        printf("Cannot open directory '%s'", dirname);
        return -1;
        
    }
    
    if(stat(destination_root, &st) == -1) {
        
        mkdir(destination_root, 0700);
        
    }
    
    //char destination_dir[1024];
    //long destination_dir_len = sizeof(char) * (strlen(destination_root) + strlen(get_filename(dirname)) + 2);
    //char destination_dir[destination_dir_len];
    
    char *destination_dir = concat_paths(destination_root, get_filename(dirname));
    
    //snprintf(destination_dir, destination_dir_len, "%s/%s", destination_root, get_filename(dirname));
    
    //concat_paths(destination_dir, destination_dir_len, destination_root, get_filename(dirname));
    
    if(stat(destination_dir, &st) == -1) {
        
        mkdir(destination_dir, 0700);
        
    }
    
    while((pDirent = readdir(pDir)) != NULL) {
        
        if(strcmp(pDirent->d_name, ".") == 0 || strcmp(pDirent->d_name, "..") == 0) {
            continue;
        }
        
        /*char current_path[1024];
        concat_paths(current_path, sizeof(current_path), dirname, pDirent->d_name);*/
        char *current_path = concat_paths(dirname, pDirent->d_name);
        
        /*char destination[1024];
        concat_paths(destination, sizeof(destination), destination_dir, pDirent->d_name);*/
        char *destination = concat_paths(destination_dir, pDirent->d_name);
        
        if(pDirent->d_type == DT_REG) {
            
            //If destination file exists
            if(stat(destination, &st) >= 0) {
                if(compare_dates(current_path, destination) < 0) {
                    return -1;
                }
            }
            copy_file(current_path, destination);
            
        } else if(pDirent->d_type == DT_DIR) {
            
            copydir(current_path, destination);
        
        }
        
    }
    closedir(pDir);
    
    return 0;
}

int compare_dates(const char *file1, const char *file2) {
    struct stat fst1, fst2;
    time_t t1, t2;
    
    if(stat(file1, &fst1) < 0 || stat(file2, &fst2) < 0) {
        return -1;
    }
    
    t1 = fst1.st_mtime;
    t2 = fst2.st_mtime;
    
    return difftime(t1, t2);
}

int copy_file(const char *from, const char *to) {
    copyfile_state_t s;
    s = copyfile_state_alloc();
    int i = copyfile(from, to, s, COPYFILE_ALL);
    
    copyfile_state_free(s);
    
    return i;
}

char *get_filename(const char* path) {
    int counter = 0;
    long index = 0;
    
    for(long i = strlen(path) - 1; i >= 0; --i) {
        counter++;
        if(path[i] == '/') {
            index = i + 1;
            break;
        }
    }
    
    char *dest = (char *) malloc(sizeof(char) * counter + 1);
    
    strcat(dest, &path[index]);
    
    return dest;
}

/*void concat_paths(char *output, long output_size, const char *base, const char *extension) {
    
    int pathlen = snprintf(output, output_size, "%s/%s", base, extension);
    output[pathlen] = 0;
    
}*/

char *concat_paths(const char *base, const char *extension) {
    int path_separator_len = 1;
    long output_size = sizeof(char) * (strlen(base) + strlen(extension) + path_separator_len + 1);
    
    char *output = malloc(output_size);
    
    int pathlen = snprintf(output, output_size, "%s/%s", base, extension);
    output[pathlen] = 0;
    
    return output;
    
}

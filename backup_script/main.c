//
//  main.c
//  backup_script
//
//  Created by Simon Olofsson on 2016-09-05.
//  Copyright © 2016 Simon Olofsson. All rights reserved.
//

#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <copyfile.h>
#import <pthread.h>

void *copy_thread(void *backup_info);
int copy_file(const char *from, const char *to);
int copydir(const char *dirname, const char *destination_root);
int compare_dates(const char *file1, const char *file2);
char *get_filename(const char *path);
void concat_paths(char *output, long output_size, const char *base, const char *extension);

struct backup_information {
    char orig_path[1024];
    char destination[1024];
};

int main(int argc, const char * argv[]) {
    const char *paths[2];
    pthread_t threads[2];
    //pthread_t pth1, pth2;
    
    paths[0] = "/Users/simpa2k/C/backup_script/backup_script/testdir";
    paths[1] = "/Users/simpa2k/C/backup_script/backup_script/testdir2";
    
    char destination[] = "/Users/simpa2k/C/backup_script/backup_script/destination";
    
    for(int i = 0; i < sizeof(paths) / sizeof(char *); i++) {
        //copydir(paths[i], destination);
        struct backup_information *info = malloc(sizeof(*info));
        
        strcpy(info->orig_path, paths[i]);
        strcpy(info->destination, destination);
    
        pthread_create(&threads[i], NULL, copy_thread, info);

    }
    
    for(int i = 0; i < sizeof(threads) / sizeof(pthread_t *); i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}

void *copy_thread(void *backup_info) {
    struct backup_information *info = backup_info;
    
    copydir(info->orig_path, info->destination);
    
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
    
    char destination_dir[1024];
    concat_paths(destination_dir, sizeof(destination_dir), destination_root, get_filename(dirname));
    
    if(stat(destination_dir, &st) == -1) {
        
        mkdir(destination_dir, 0700);
        
    }
    
    while((pDirent = readdir(pDir)) != NULL) {
        
        if(strcmp(pDirent->d_name, ".") == 0 || strcmp(pDirent->d_name, "..") == 0) {
            continue;
        }
        
        char current_path[1024];
        concat_paths(current_path, sizeof(current_path), dirname, pDirent->d_name);
        
        char destination[1024];
        concat_paths(destination, sizeof(destination), destination_dir, pDirent->d_name);
        
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

void concat_paths(char *output, long output_size, const char *base, const char *extension) {
    
    int pathlen = snprintf(output, output_size, "%s/%s", base, extension);
    output[pathlen] = 0;
    
}

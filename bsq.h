#ifndef BSQ_H
#define BSQ_H
#include <stdio.h>

typedef struct s_params {
    int rows;
    char empty;
    char obstacle;
    char full;
}   t_params;

int process_file(const char *path, int is_last);
int process_stream(FILE *f, int is_last);

#endif


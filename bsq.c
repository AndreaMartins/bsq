#define _GNU_SOURCE
#include "bsq.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

/* ============================= */
/*         ERROR PRINT           */
/* ============================= */

static void print_map_error(void)
{
    fputs("map error\n", stderr);
}

/* Remove trailing '\n' */
static ssize_t trim_newline(char *buf, ssize_t len)
{
    if (len > 0 && buf[len - 1] == '\n')
    {
        buf[len - 1] = '\0';
        return len - 1;
    }
    return len;
}

/* ============================= */
/*       PARSE FIRST LINE        */
/* ============================= */
#include <ctype.h>

static int parse_first_line(char *line, t_params *p)
{
    char *ptr = line;
    int rows = 0;

    // ======== 1. Leer número de filas ========
    if (!isdigit(*ptr)) return -1; // primera posición debe ser dígito

    while (isdigit(*ptr)) {
        rows = rows * 10 + (*ptr - '0'); // convertir carácter a número
        ptr++;
    }

    if (rows <= 0) return -1;

    // Saltar espacios/tabs hasta el siguiente campo
    while (*ptr && isspace((unsigned char)*ptr)) ptr++;

    // ======== 2. Leer caracter 'empty' ========
    if (*ptr == '\0') return -1;
    char empty = *ptr++;
    while (*ptr && isspace((unsigned char)*ptr)) ptr++;

    // ======== 3. Leer caracter 'obstacle' ========
    if (*ptr == '\0') return -1;
    char obstacle = *ptr++;
    while (*ptr && isspace((unsigned char)*ptr)) ptr++;

    // ======== 4. Leer caracter 'full' ========
    if (*ptr == '\0') return -1;
    char full = *ptr++;

    // ======== 5. Validar que los caracteres sean distintos ========
    if (empty == obstacle || empty == full || obstacle == full)
        return -1;

    // ======== 6. Guardar en la estructura ========
    p->rows = rows;
    p->empty = empty;
    p->obstacle = obstacle;
    p->full = full;

    return 0; // éxito
}


/* ============================= */
/*         READ MAP LINES        */
/* ============================= */

static int read_map(FILE *f, int expected_rows, char ***out, int *cols)
{
    char **map = malloc(expected_rows * sizeof(char *)); // Reservamos todo desde el inicio
    if (!map)
        return -1;

    char *line = NULL;
    size_t linecap = 0;
    ssize_t len;
    int width = -1;

    for (int r = 0; r < expected_rows; r++)
    {
        // Leer una línea del archivo
        len = getline(&line, &linecap, f);
        if (len == -1)
            goto fail;

        // Quitar el salto de línea
        len = trim_newline(line, len);
        if (len <= 0)
            goto fail;

        // Determinar ancho del mapa con la primera fila
        if (width == -1)
            width = (int)len;
        else if (len != width) // Validar que todas las filas tengan el mismo ancho
            goto fail;

        // Reservar memoria para la fila y copiarla
        map[r] = malloc(len + 1);
        if (!map[r])
            goto fail;

        memcpy(map[r], line, len + 1);
    }

    free(line);

    *out = map;    // Pasar puntero a la matriz
    *cols = width; // Pasar ancho de las filas
    return expected_rows;

fail:
    free(line);
    for (int i = 0; i < expected_rows; i++)
        free(map[i]);
    free(map);
    return -1;
}

/* ============================= */
/*      VALIDATE MAP CHARS       */
/* ============================= */

static int validate_chars(char **map, int rows, int cols, t_params *p)
{
    for (int y = 0; y < rows; y++)
        for (int x = 0; x < cols; x++)
            if (map[y][x] != p->empty && map[y][x] != p->obstacle)
                return -1;
    return 0;
}

/* ============================= */
/*        SOLVE AND PRINT        */
/* ============================= */

static int solve_and_print(char **map, int rows, int cols, t_params *p)
{
    int *prev = calloc(cols + 1, sizeof(int));
    int *curr = calloc(cols + 1, sizeof(int));
    if (!prev || !curr)
    {
        free(prev); free(curr);
        return -1;
    }

    int best_size = 0, best_y = 0, best_x = 0;

    for (int y = 0; y < rows; y++)
    {
        for (int x = 1; x <= cols; x++)
        {
            if (map[y][x - 1] == p->empty)
            {
                int a = prev[x];
                int b = curr[x - 1];
                int c = prev[x - 1];
                int m = a < b ? a : b;
                if (c < m) m = c;
                curr[x] = m + 1;

                if (curr[x] > best_size)
                {
                    best_size = curr[x];
                    best_y = y - best_size + 1;
                    best_x = x - best_size;
                }
            }
            else
                curr[x] = 0;
        }

        int *tmp = prev;
        prev = curr;
        curr = tmp;
    }

    free(prev);
    free(curr);

    /* Fill the best square */
    for (int y = best_y; y < best_y + best_size; y++)
        for (int x = best_x; x < best_x + best_size; x++)
            map[y][x] = p->full;

    /* Print result */
    for (int y = 0; y < rows; y++)
    {
        fputs(map[y], stdout);
        fputc('\n', stdout);
    }

    return 0;
}

/* ============================= */
/*        PROCESS STREAM         */
/* ============================= */

int process_stream(FILE *f, int is_last)
{
    char *first = NULL;
    size_t cap = 0;
    ssize_t len = getline(&first, &cap, f);
    if (len == -1)
    {
        free(first);
        print_map_error();
        return -1;
    }

    len = trim_newline(first, len);
    if (len <= 0)
    {
        free(first);
        print_map_error();
        return -1;
    }

    t_params p;
    if (parse_first_line(first, &p) != 0)
    {
        free(first);
        print_map_error();
        return -1;
    }

    free(first);

    char **map = NULL;
    int cols = 0;
    int rows = read_map(f, p.rows, &map, &cols);

    if (rows != p.rows || cols <= 0)
    {
        if (map)
            for (int i = 0; i < rows; i++) free(map[i]);
        free(map);
        print_map_error();
        return -1;
    }

    if (validate_chars(map, rows, cols, &p) != 0)
    {
        for (int i = 0; i < rows; i++) free(map[i]);
        free(map);
        print_map_error();
        return -1;
    }

    if (solve_and_print(map, rows, cols, &p) != 0)
    {
        for (int i = 0; i < rows; i++) free(map[i]);
        free(map);
        print_map_error();
        return -1;
    }

    for (int i = 0; i < rows; i++) free(map[i]);
    free(map);

    if (!is_last)
        putchar('\n');

    return 0;
}

/* ============================= */
/*         PROCESS FILES         */
/* ============================= */

int process_file(const char *path, int is_last)
{
    FILE *f = fopen(path, "r");
    if (!f)
    {
        print_map_error();
        if (!is_last) putchar('\n');
        return -1;
    }

    int r = process_stream(f, is_last);
    fclose(f);
    return r;
}

/* ============================= */
/*             MAIN              */
/* ============================= */

int main(int argc, char **argv)
{
    if (argc == 1)
    {
        process_stream(stdin, 1);
        return 0;
    }

    for (int i = 1; i < argc; i++)
        process_file(argv[i], (i == argc - 1));

    return 0;
}


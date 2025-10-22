#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <unistd.h>

#define AIR_MULT   0.009
// #define WATER_MULT 1.0
#define LAND_MULT  0.002
#define DECAY      0.8

#define MIN(a,b) (a<b?a:b)
#define MAX(a,b) (a>b?a:b)
#define CLAMP(a,mn,mx) MIN(mx,MAX(mn,a))

typedef enum {
    E_TILE_NONE,
    E_TILE_GROUND,
    E_TILE_WATER,
    TILE_TYPES_TOTAL
} e_tile_type_t;

typedef struct {
    e_tile_type_t type;
    float elevation;
} tile_t;

typedef struct {
    float air_contaminant, water_contaminant, land_contaminant;
} cell_t;

#define ROWS 100
#define COLS 100
#define GRID_SIZE (ROWS*COLS)
#define RC2I(r,c) (r*COLS+c)
#define I2R(i) ((int)i/COLS)
#define I2C(i) ((int)i%COLS)
#define I2RC(i) I2R(i),I2C(i)
typedef float grid_t[ROWS][COLS];

typedef struct {
    struct {
        // Multiplier for effectiveness
        float speed;
        // Direction in radians
        float direction;
    } wind;
    tile_t tiles[ROWS][COLS];
} map_t;

bool step_cell(grid_t *in, grid_t *out, int row, int col, map_t *map) {
    tile_t my = map->tiles[row][col];
    float wind_r = (float) row - asin(map->wind.direction)*map->wind.speed,
          wind_c = (float) col - acos(map->wind.direction)*map->wind.speed;
    float total = DECAY * (*in)[row][col];
    for(int ri = MAX(0,row-1); ri < MIN(ROWS, row+2); ri++) {
        for(int ci = MAX(0,col-1); ci < MIN(COLS, col+2); ci++) {
            tile_t their = map->tiles[ri][ci];
            total += AIR_MULT
                   * sqrtf(powf(ri-wind_r,2)+powf(ci-wind_c,2))
                   * fabsf(their.elevation - my.elevation + 1)
                   * (*in)[ri][ci];
            total += LAND_MULT*(*in)[ri][ci];
            // total += AIR_MULT * in[ri][ci]->air_contaminant * my.elevation / their.elevation;
            // total += LAND_MULT * in[ri][ci]->land_contaminant;
            // if(my.type == E_TILE_WATER && their.type == E_TILE_WATER)
            //     total += WATER_MULT * in[ri][ci]->water_contaminant * my.elevation / their.elevation;
            // else
            //     total += WATER_MULT * LAND_MULT * in[ri][ci]->water_contaminant * my.elevation / their.elevation;
        }
    }
    (*out)[row][col] = total;
    return fabsf(total) < 0.01;
}

void print_grid(grid_t *a) {
    // printf("=========\n");
    float m = -1000;
    for(int i = 0; i < GRID_SIZE; i++) {
        float v = (*a)[I2R(i)][I2C(i)];
        if(v > m) m = v;
    }
    m = 255 / m;
    printf("\033[H\033[3J");
    for(int r = 0; r < ROWS; r+=2) {
        for(int c = 0; c < COLS; c++) {
            printf("\033[38;2;%d;%d;%dm\033[48;2;%d;%d;%dm▀",
                ((*a)[r][c]>255   || (*a)[r][c]<0)*255,   CLAMP((int)(*a)[r][c]*20,   0, 255), 0,
                ((*a)[r+1][c]>255 || (*a)[r+1][c]<0)*255, CLAMP((int)(*a)[r+1][c]*20, 0, 255), 0
            );
            // printf("%.2f\t", (*a)[r][c]);
        }
        printf("\033[0m\n");
    }
}

void print_map(map_t m) {
    // printf("=========\n");
    // printf("\033[H\033[3J");
    printf("Wind:\n\tSpeed: %f\n\tDirection: %f\n", m.wind.speed, m.wind.direction);
    for(int r = 0; r < ROWS; r+=2) {
        for(int c = 0; c < COLS; c++) {
            printf("\033[38;2;%d;%d;%dm\033[48;2;%d;%d;%dm▀",
                (int)(m.tiles[r][c].elevation*255),   0,     0,
                (int)(m.tiles[r+1][c].elevation*255), 0,     0
            );
            // printf("%.2f\t", (*a)[r][c]);
        }
        printf("\033[0m\n");
    }
}

int main(int argc, char** argv) {
    grid_t a,b;
    for(int r = 0; r < ROWS; r++) {
        for(int c = 0; c < COLS; c++) {
            a[r][c] = 0;
            b[r][c] = 0;
        }
    }
    bool parity = 1;

    map_t map;
    map.wind.direction = 0.8;
    map.wind.speed     = 0.1;
    for(int r = 0; r < ROWS; r++) {
        for(int c = 0; c < COLS; c++) {
            float e = 0.25 * cos(0.1 * (30-c)) + 0.25 * cos(0.5 * r) + 0.5;
            if(e < 0 || e > 1)
                printf("Bad value at %d,%d (%f)\n", c, r, e);
            map.tiles[r][c] = (tile_t){
                // .elevation = 1.25*cos(0.8*(float)r)+1.25*cos(0.3*(float)c),
                .elevation = e,
                .type = E_TILE_GROUND,
            };
        }
    }   
    if(argc > 2) {
        print_map(map);
        return 0;
    }

    a[ROWS/2][COLS/2] = 100.0;

    int gen = 0;
    bool done = 0;
    while(!done) {
        map.wind.direction = 0.1;
        map.wind.speed     = 6.28/360 * 90;
        done = 1;
        // #pragma omp parallel for num_threads(8)
        for(int i = 0; i < GRID_SIZE; i++) {
            if(parity){
                done &= step_cell(&a, &b, I2RC(i), &map);
            } else {
                done &= step_cell(&b, &a, I2RC(i), &map);
            }
        }

        usleep(15000);
        print_grid(parity?&a:&b);
        printf("\033[KGeneration: %d\n", gen++);
        parity = !parity;
    }
}
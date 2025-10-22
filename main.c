#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <math.h>
#include <unistd.h>

#define AIR_MULT   0.3
// #define WATER_MULT 1.0
#define LAND_MULT  0.01
#define DECAY      0.1

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

#define ROWS 30
#define COLS 30
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
    float wind_x = acos(map->wind.direction)*map->wind.speed,
          wind_y = asin(map->wind.direction)*map->wind.speed;
    float total = DECAY*(*in)[row][col];
    for(int ri = MAX(0,row-1); ri < MIN(ROWS-1, row+1); ri++) {
        for(int ci = MAX(0,col-1); ci < MIN(COLS-1, col+1); ci++) {
            tile_t their = map->tiles[ri][ci];
            total += AIR_MULT
                   * sqrtf(powf(row-ri-wind_y,2)+powf(col-ci-wind_x,2))
                   * fabsf(their.elevation - my.elevation)
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
    printf("\033[H\033[3J");
    for(int r = 0; r < ROWS; r+=2) {
        for(int c = 0; c < COLS; c++) {
            printf("\033[38;2;%d;%d;%dm\033[48;2;%d;%d;%dm▀", ((*a)[r][c]>255)*255, CLAMP((int)(*a)[r][c]*10,0,255), 0, ((*a)[r+1][c]>255)*255, CLAMP((int)(*a)[r+1][c]*10, 0, 255), 0);
            // printf("%.2f\t", (*a)[r][c]);
        }
        printf("\033[0m\n");
    }
}

int main(void) {
    grid_t a,b;
    for(int r = 0; r < ROWS; r++) {
        for(int c = 0; c < COLS; c++) {
            a[r][c] = 0;
            b[r][c] = 0;
        }
    }
    bool parity = 1;

    map_t map;
    for(int r = 0; r < ROWS; r++) {
        for(int c = 0; c < COLS; c++) {
            map.tiles[r][c] = (tile_t){
                .elevation = 1.25*cos(0.8*(float)r)+1.25*cos(0.3*(float)c),
                .type = E_TILE_GROUND,
            };
        }
    }

    a[1][1] = 100.0;

    int gen = 0;
    bool done = 0;
    while(!done) {
        map.wind.direction = 0.8;
        map.wind.speed     = 0.1;
        done = 1;
        #pragma omp parallel for num_threads(8)
        for(int i = 0; i < GRID_SIZE; i++) {
            if(parity){
                done &= step_cell(&a, &b, I2RC(i), &map);
            } else {
                done &= step_cell(&b, &a, I2RC(i), &map);
            }
        }

        usleep(150000);
        print_grid(parity?&a:&b);
        printf("\033[KGeneration: %d\n", gen++);
        parity = !parity;
    }
}
#include <stdio.h>
#include <stdlib.h>

#define rows 20
#define cols 20

#define source_strength 100

#define map_name "maps/1_hill.txt"

int load_map(char grid[rows][cols]) {
    FILE *fp = fopen(map_name, "r");
    if (!fp) return 1;

    char ch;

    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            int ch = fgetc(fp);
            grid[r][c] = (char) ch;
        }
        fgetc(fp);
    }
    fclose(fp);
    return 0;
}

void print_map(char grid[rows][cols]) {
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols; c++) {
            printf("%c", grid[r][c]);
        }
        printf("\n");
    }
}

int main(int argc, char* argv[]) {
    char grid[rows][cols];

    int status = load_map(grid);

    if (status == 0) {
        print_map(grid);
    } else {
        printf("Error loading map...\n");
        exit(1);
    }
}
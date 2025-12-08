// Maze Maps Definition
// Legend:
// Cell contents: '.' = empty, 'P' = player start, 'E' = end, 'M' = marker
// Wall characters: 
//   '-' = horizontal wall
//   '|' = vertical wall  
//   '+' = corner/junction
//   ' ' = passable (no wall)
// 
// Grid format (13x13):
// Odd rows (1,3,5,7,9,11): cell row with vertical walls between
//   Format: + cell | cell | cell | cell | cell | cell +
// Even rows (2,4,6,8,10,12): horizontal walls below cells
//   Format: + wall + wall + wall + wall + wall + wall +
//
// Example:
// +P+ +M+ +.+ +.+ +.+ +.+    <- row of cells with vertical walls
// +-++++-+++++++++++++++    <- horizontal walls below each cell

#pragma once

namespace MazeMaps {

// Map 1
const char map1_layout[13][14] = {
    "+-----+-----+",
    "|. P .|. . .|",
    "| +-- |  ---+",
    "|M|. .|. . .|",
    "| + --+--- .|",
    "|.|. .|. . M|",
    "| +-    +-  |",
    "|.|. . .|. .|",
    "| +---+-+-+ |",
    "|. . .|. .|.|",
    "| --+   +-+ |",
    "|. .|. .|. E|",
    "+---+---+---+",
};

// Map 2
const char map2_layout[13][14] = {
    "+-----+-----+",
    "|. . .|. . P|",
    "+-- +-+    -|",
    "|. .|. .|M .|",
    "| +-+ +-+- .|",
    "|.|. .|. . .|",
    "|   +-+ +-+ |",
    "|. M|. .|.|.|",
    "| +-+ +-+ | |",
    "|.|.|.|. .|.|",
    "| |   |  -+ |",
    "|.|. .|. . E|",
    "+-+---+-----+",
};

// Map 3
const char map3_layout[13][14] = {
    "+-------+---+",
    "|. . .|.|. .|",
    "| +-+ |   | |",
    "|.|.|.|. .|.|",
    "+-+ | +-+-+ |",
    "|. .|.|. .|.|",
    "|   | | | | |",
    "|.|.|.|M|.|M|",
    "| |   | | | |",
    "|.|. .|.|.|.|",
    "| +---+ |   |",
    "|. . . .|. .|",
    "+-------+---+",
};

// Map 4
const char map4_layout[13][14] = {
    "+---+-------+",
    "|M .|. . . .|",
    "| | +-----  |",
    "|.|.|. . . .|",
    "| | | +---+ |",
    "|.|. .|. .|.|",
    "| +---+ --+ |",
    "|M|. . . . .|",
    "| +-------+ |",
    "|. . . . .|.|",
    "| ----+-- | |",
    "|. . .|. .|.|",
    "+-----+---+-+",
};

// Map 5
const char map5_layout[13][14] = {
    "+-----------+",
    "|. . . . . .|",
    "+-------  | |",
    "|. . . . .|.|",
    "| --+-- +---+",
    "|. .|. .|M .|",
    "|   +---+   |",
    "|.|. . .|.|.|",
    "| +---  +-+ |",
    "|.|. . . .|.|",
    "| |  -----+ |",
    "|.|. . M . .|",
    "+-+---------+",
};

// Map 6
const char map6_layout[13][14] = {
    "+-+---+-----+",
    "|.|. .|. M .|",
    "| |   +--   |",
    "|.|.|.|. .|.|",
    "|   | | +-+ |",
    "|. .|.|.|. .|",
    "|  -+-+ | +-+",
    "|. .|. .|.|.|",
    "+-  |   |   |",
    "|. .|M|.|. .|",
    "|  -+-+ +-  |",
    "|. . . .|. .|",
    "+-----------+",
};

// Map 7
const char map7_layout[13][14] = {
    "+-------+---+",
    "|. M . .|. .|",
    "| +---+     |",
    "|.|. .|. .|.|",
    "|   +-+-+-+ |",
    "|. .|. .|. .|",
    "+---+  -+ +-+",
    "|. .|. . .|.|",
    "|   |  ---+ |",
    "|.|.|. . .|.|",
    "| +-+---    |",
    "|. M . . . .|",
    "+-----------+",
};

// Map 8
const char map8_layout[13][14] = {
    "+-+-----+---+",
    "|.|. . M|. .|",
    "|    -+     |",
    "|. . .|. .|.|",
    "| +---+---+ |",
    "|.|. . . .|.|",
    "| |  -+-    |",
    "|.|. M|. . .|",
    "| +-+ +-----+",
    "|.|.|. . . .|",
    "|   +-------+",
    "|. . . . . .|",
    "+-----------+",
};

// Map 9
const char map9_layout[13][14] = {
    "+-+---------+",
    "|.|. . . . .|",
    "| | +---+   |",
    "|.|.|M .|.|.|",
    "|     +-+ | |",
    "|. . .|. .|.|",
    "| +-+-+ +-+ |",
    "|.|.|. .|. .|",
    "| | | +-+-+ |",
    "|M|.|.|. .|.|",
    "|   |   | +-|",
    "|. .|. .|. .|",
    "+-------+---+",
};


// Helper function to parse map and extract marker positions
inline void parse_map(const char layout[13][14], int& m1x, int& m1y, int& m2x, int& m2y) {
    bool found_m1 = false;
    // Scan cell positions (odd rows 1,3,5,7,9,11 and odd columns 1,3,5,7,9,11)
    for (int row = 1; row <= 11; row += 2) {
        for (int col = 1; col <= 11; col += 2) {
            char c = layout[row][col];
            int x = (col - 1) / 2;  // Convert to 0-5 grid
            int y = (row - 1) / 2;  // Convert to 0-5 grid
            
            if (c == 'M') {
                if (!found_m1) { m1x = x; m1y = y; found_m1 = true; }
                else { m2x = x; m2y = y; }
            }
        }
    }
}

// Helper to check if there's a wall between two adjacent cells
inline bool has_wall(const char layout[13][14], int x1, int y1, int x2, int y2) {
    // Check if cells are adjacent
    int dx = (x1 > x2) ? (x1 - x2) : (x2 - x1);
    int dy = (y1 > y2) ? (y1 - y2) : (y2 - y1);
    if (dx + dy != 1) return true;  // Not adjacent = blocked
    
    // Calculate wall position in the grid
    if (x1 == x2) {  // Vertical movement - check horizontal wall
        int wall_row = (y1 < y2 ? y1 : y2) * 2 + 2;  // Wall is below the upper cell
        int wall_col = x1 * 2 + 1;
        char wall_char = layout[wall_row][wall_col];
        return (wall_char == '-' || wall_char == '+');  // wall exists
    } else {  // Horizontal movement - check vertical wall
        int wall_row = y1 * 2 + 1;
        int wall_col = (x1 < x2 ? x1 : x2) * 2 + 2;  // Wall is right of the left cell
        char wall_char = layout[wall_row][wall_col];
        return (wall_char == '|' || wall_char == '+');  // wall exists
    }
}

} // namespace MazeMaps

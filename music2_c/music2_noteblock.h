//*****************************************************************************
// music2_noteblock.h.
// Scope: Private to music2 program.
// Bare-bones .h file to facilitate linking. See the .c file for descriptions.
//*****************************************************************************

#pragma once

#include <stdlib.h> // malloc

#define NOTEBLOCK_WIDTH   (5)
#define NOTEBLOCK_HEIGHT (16)
struct noteblock {
    struct noteblock* pNext;
    char text[NOTEBLOCK_HEIGHT][NOTEBLOCK_WIDTH];
};
#define ROW_HI_B (15)
#define ROW_HI_A (14)
#define ROW_HI_G (13)
#define ROW_HI_F (12)
#define ROW_HI_E (11)
#define ROW_HI_D (10)
#define ROW_HI_C  (9)
#define ROW_MD_B  (8)
#define ROW_LO_A  (7)
#define ROW_LO_G  (6)
#define ROW_LO_F  (5)
#define ROW_LO_E  (4)
#define ROW_LO_D  (3)
#define ROW_LO_C  (2)
#define ROW_LO_B  (1)
#define ROW_TEXT  (0)
inline char* get_ptr_to_text (struct noteblock* pNoteblock){
    return &((pNoteblock->text)[0][0]);
}
inline char* get_ptr_to_row_from_noteblock (struct noteblock* pNoteblock, int row){
    return &((pNoteblock->text)[row][0]);
}
inline char* get_ptr_to_row_from_text (char* pText, int row){
    return pText + (row * NOTEBLOCK_WIDTH);
}
void draw_row (char* pText, int row, char c0, char c1, char c2, char c3, char c4);
void draw_row_raw (char* pText, int row, char c0, char c1, char c2, char c3, char c4);
inline void draw_row_error (char* pText, int row){
    draw_row_raw (pText, row, 'E', 'R', 'R', 'O', 'R');
}
inline struct noteblock* allocate_noteblock (){
    return malloc (sizeof (struct noteblock));
}
void free_noteblocks (struct noteblock* pNoteblock);
unsigned int count_noteblocks (struct noteblock* pNoteblock);

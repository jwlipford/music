//*******************************************************************************************
// music2_noteblock.c
// This file defines the noteblock struct - data structure, constants, and simple functions.
//*******************************************************************************************


// External inclusions
#include <stddef.h> // NULL
#include <stdlib.h> // malloc, free


//********************************************************************************************************************
// Noteblock structure and associated constants.
// A noteblock contains a 16-row, 5-column array of text used by a note, time signature, key signature, barline, etc.
// It also has a pointer to the next noteblock in a linked list of noteblocks.
//********************************************************************************************************************

#define NOTEBLOCK_WIDTH   (5)
#define NOTEBLOCK_HEIGHT (16)

struct noteblock {
    // Next noteblock in linked list
    struct noteblock* pNext;
    
    // 2D array of text for the noteblock. Not a pointer.
    // If <5 characters wide, terminate with \0, but make sure each row is same length.
    char text[NOTEBLOCK_HEIGHT][NOTEBLOCK_WIDTH];
};

// ROW constants (some unused, defining all)
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



//*****************************************************************
// "Get pointer" functions to get pointers to parts of a noteblock
//*****************************************************************

// Given a noteblock, get a pointer to its text array
inline char* get_ptr_to_text (
    struct noteblock* pNoteblock // Pointer to a noteblock
    // Returns pointer to *pNoteblock's 2-dimensional array of text
){
    return &((pNoteblock->text)[0][0]);
}


// Given a noteblock and a row number, get a pointer to that row in the noteblock's text array
inline char* get_ptr_to_row_from_noteblock (
    struct noteblock* pNoteblock, // Pointer to a noteblock
    int               row         // Row number (0-15)
    // Returns pointer to the row in *pNoteblock's 2-dimensional array of text
){
    return &((pNoteblock->text)[row][0]);
}


// Given a noteblock's text array and a row number, get a pointer to that row in the text array
inline char* get_ptr_to_row_from_text (
    char* pText, // Pointer to a noteblock's 2-dimensional array of text
    int   row    // Row number (0-15)
    // Returns pointer to the row in *pText
){
    return pText + (row * NOTEBLOCK_WIDTH);
    // We can't just do pText[row] because the compiler doesn't know that each row is 5 bytes.
    // pText[row] is not a pointer to an array of characters, it is an actual character in the 2D array.
}



//***************************************************************
// "Draw row" functions to draw within an existing noteblock row
//***************************************************************

// Overwrite some characters in a row of a noteblock
void draw_row (
    char* pText, // (Pointer to) a noteblock's 2D array text
    int   row,   // Row number (0-15)
    // Below: Characters to copy to the 5-character long row. Pass 1 to keep existing char.
    char c0, char c1, char c2, char c3, char c4
){
    char* pTextRow = get_ptr_to_row_from_text (pText, row);
    if (c0 != 1) pTextRow[0] = c0;
    if (c1 != 1) pTextRow[1] = c1;
    if (c2 != 1) pTextRow[2] = c2;
    if (c3 != 1) pTextRow[3] = c3;
    if (c4 != 1) pTextRow[4] = c4;
}


// Overwrite all characters in a row of a noteblock.
void draw_row_raw (
    char* pText, // (Pointer to) a noteblock's 2D array text
    int   row,   // Row number (0-15)
    // Below: Characters to copy to the 5-character long row. 1 is treated like a normal character.
    char c0, char c1, char c2, char c3, char c4
){
    char* pTextRow = get_ptr_to_row_from_text (pText, row);
    pTextRow[0] = c0; pTextRow[1] = c1; pTextRow[2] = c2; pTextRow[3] = c3; pTextRow[4] = c4;
}


// Overwrite a row of a noteblock with the string "ERROR".
inline void draw_row_error (
    char* pText, // (Pointer to) a noteblock's 2D array text
    int   row    // Row number (0-15)
){
    draw_row_raw (pText, row, 'E', 'R', 'R', 'O', 'R');
}



//**************************************
// Allocate, free, and count noteblocks
//**************************************

// Allocate memory on the heap for a new noteblock
inline struct noteblock* allocate_noteblock () {
    return malloc (sizeof (struct noteblock));
}


// Deallocate memory for a noteblock and following noteblocks
void free_noteblocks (
    struct noteblock* pNoteblock // Pointer to noteblock. It and following will be freed.
){
    while (pNoteblock != NULL) {
        struct noteblock* pNextNoteblock = pNoteblock->pNext;
        free (pNoteblock);
        pNoteblock = pNextNoteblock;
    }
}


// Count noteblocks including and following a given noteblock
unsigned int count_noteblocks (
    struct noteblock* pNoteblock // Pointer to a noteblock. It and following will be counted.
){
    unsigned int count = 0;
    while (pNoteblock != NULL) {
        pNoteblock = pNoteblock->pNext;
        ++count;
    }
    return count;
}

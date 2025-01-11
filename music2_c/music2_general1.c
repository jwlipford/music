//**************************************************************************************
// music2_general1.c
// This file contains general logic that individual byte group types' logic depends on.
//**************************************************************************************


// Internal inclusions
#include "music2_noteblock.h"


//*************
// Music staff
//*************

// Standard staff bitstring, used to produce a noteblock's staff without ledger lines or other variations.
// Format: 1 represents a line row, 0 represents a space row.
// Most significant bit (left) represents ROW_HI_B, least significant bit (right) represents ROW_TEXT.
const unsigned short STD_STAFF_BITSTR = 0b0001010101010000;

// Draw the staff (spaces and lines) in a noteblock.
void draw_staff (
    char*          pText,      // (Pointer to) a noteblock's 2D array of text, in which to draw the staff.
    int            width,      // Width of noteblock. If less than 5, remaining column(s) will be filled with '\0's.
    unsigned short staffBitstr // Staff bitstring. For noteblock types other than note, always use STD_STAFF_BITSTR.
                   // For notes, see staff_bitstr_for_note.
){
    // Create two strings: cs for drawing to space rows, cL for drawing to line rows
    char cs[NOTEBLOCK_WIDTH] = {' ', ' ', ' ', ' ', ' '};
    char cL[NOTEBLOCK_WIDTH] = {'-', '-', '-', '-', '-'};
    for (int i = width; i < NOTEBLOCK_WIDTH; ++i) {
        cs[i] = '\0';
        cL[i] = '\0';
    }
    // Draw rows
    for (int i = ROW_TEXT; i <= ROW_HI_B; ++i) {
        if ((staffBitstr >> i) & 0b1) {
            draw_row_raw (pText, i, cL[0], cL[1], cL[2], cL[3], cL[4]);
        }
        else {
            draw_row_raw (pText, i, cs[0], cs[1], cs[2], cs[3], cs[4]);
        }
    }
}



//**************
// ROW location
//**************

// Whether the row is a space adjacent to the Middle B line
inline int row_is_beside_mid_B (
    int row // Row number (0-15)
){
    return (row == ROW_LO_A) || (row == ROW_HI_C);
}


// Whether a row is a line at the top or bottom of the staff - the Low E line or High F line
inline int row_is_edge (
    int row // Row number (0-15)
){
    return (row == ROW_LO_E) || (row == ROW_HI_F);
}


// Whether the row is a space (meaning odd numbered - rows that could have ledger lines don't count)
inline int row_is_space (
    int row // Row number (0-15)
){
    return row % 2;
}

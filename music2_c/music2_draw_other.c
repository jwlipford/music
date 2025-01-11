//*******************************************************************************************
// music2_draw_other.c
// This file contains logic for drawing notation from byte group types other than type note:
// dynamics text, time change, key change, barline, clef.
//*******************************************************************************************


// External inclusions
#include <stddef.h> // NULL

// Internal inclusions
#include "music2_general1.h"
#include "music2_noteblock.h"


//*************************
// Make and draw functions
//*************************

// The sixteen characters that can be represented by a four-bit dynamics text encoding.
// Bits 0,14,15 are invalid/unused, so they are 'E' for error.
const char DYNAMICS_CHARACTERS[16] = { 'E', '\0', ' ', '<', '>', '.', 'c', 'd', 'e', 'f', 'm', 'p', 'r', 's', 'E', 'E' };

// Draw the dynamics text row (bottom row, 0) in a noteblock
void draw_dynamics_text_row (
    char*         pText, // (Pointer to) a noteblock's 2D array of text, in which to draw the dynamics text.
    unsigned char byte1, // Bits 1-8 of dynamics text encoding. Bits 1-4 should be 1000.
    unsigned char byte2, // Bits 9-16 of dynamics text encoding.
    unsigned char byte3  // Bits 17-24 of dynamics text encoding.
){
    unsigned char charBits;
    charBits = byte1 / 16; // Bits 5-8
    char c0 = DYNAMICS_CHARACTERS[charBits];
    charBits = byte2 % 16; // Bits 9-12
    char c1 = DYNAMICS_CHARACTERS[charBits];
    charBits = byte2 / 16; // Bits 13-16
    char c2 = DYNAMICS_CHARACTERS[charBits];
    charBits = byte3 % 16; // Bits 17-20
    char c3 = DYNAMICS_CHARACTERS[charBits];
    charBits = byte3 / 16; // Bits 21-24
    char c4 = DYNAMICS_CHARACTERS[charBits];
    draw_row_raw (pText, ROW_TEXT, c0, c1, c2, c3, c4);
}


// Draw one row of a barline in a noteblock
void draw_barline_row (
    char*         pText, // (Pointer to) a noteblock's 2D array text
    int           row,   // Row number (0-15)
    unsigned char byte   // Bits 1-8 of barline encoding. Bits 5-7 are relevant here.
){
    switch (byte) {
        case 0b00000100: // Single barline
            if (row_is_edge (row)) draw_row (pText, row, 1, '+', 1, '\0', '\0');
            else                   draw_row (pText, row, 1, '|', 1, '\0', '\0');
            break;
        case 0b00010100: // Double barline
            if (row_is_edge (row)) draw_row (pText, row, 1, '+', '+', '\0', '\0');
            else                   draw_row (pText, row, 1, '|', '|', '\0', '\0');
            break;
        case 0b00100100: // Double barline with left repeat
            if (row_is_beside_mid_B (row)) draw_row (pText, row, 1, '0', '|', '|', 1);
            else if (row_is_edge (row))    draw_row (pText, row, 1, 1, '+', '+', 1);
            else                           draw_row (pText, row, 1, 1, '|', '|', 1);
            break;
        case 0b00110100: // Double barline with right repeat
            if (row_is_beside_mid_B (row)) draw_row (pText, row, 1, '|', '|', '0', 1);
            else if (row_is_edge (row))    draw_row (pText, row, 1, '+', '+', 1, 1);
            else                           draw_row (pText, row, 1, '|', '|', 1, 1);
            break;
        case 0b01000100: // Double barline with left and right repeats
            if (row_is_beside_mid_B (row)) draw_row (pText, row, '0', '|', '|', '0', '\0');
            else if (row_is_edge (row))    draw_row (pText, row, 1, '+', '+', 1, '\0');
            else                           draw_row (pText, row, 1, '|', '|', 1, '\0');
            break;
        case 0b01010100: // Blank column (not actually a barline)
            draw_row (pText, row, 1, '\0', '\0', '\0', '\0');
            break;
        default: // Invalid
            draw_row_error (pText, row);
            break;
    }
}


// Make a time signature noteblock
struct noteblock* make_time_signature (
    unsigned char byte // Bits 1-8 of time signature encoding. Bits 3-8 are relevant here.
    // Returns pointer to new noteblock.
){
    int topNum = (byte / 16) + 1; // In range 1-16
    int btmNum = 1 << ((byte / 4) % 4); // In {1,2,4,8}
    int topIs2Digits = (topNum > 9);

    struct noteblock* pNoteblock = allocate_noteblock ();
    if (pNoteblock == NULL) { return NULL; }
    pNoteblock->pNext = NULL;
    char* pText = get_ptr_to_text (pNoteblock);
    draw_staff (pText, (topIs2Digits ? 4 : 3), STD_STAFF_BITSTR);

    if (topIs2Digits) {
        draw_row_raw (pText, ROW_HI_C, ' ', '1', '0' + (topNum % 10), ' ', '\0');
        draw_row_raw (pText, ROW_LO_A, ' ', ' ', '0' + btmNum, ' ', '\0');
    }
    else {
        draw_row_raw (pText, ROW_HI_C, ' ', '0' + topNum, ' ', '\0', '\0');
        draw_row_raw (pText, ROW_LO_A, ' ', '0' + btmNum, ' ', '\0', '\0');
    }

    return pNoteblock;
}


// Rows in the order that we want to loop through them while making our key signature
const unsigned char KEY_SIGNATURE_ROWS[11] =
{ ROW_LO_F, ROW_MD_B, ROW_HI_E, ROW_LO_E, ROW_LO_A, ROW_HI_D, ROW_HI_G, ROW_LO_D, ROW_LO_G, ROW_HI_C, ROW_HI_F };

// Make a key signature noteblock
struct noteblock* make_key_signature (
    unsigned short bits01to16, // Bits 1-16 of key signature encoding. Bits 4-14 are relevant here.
    unsigned short bits17to32  // Bits 17-32 of key signature encoding. Bits 20-30 are relevant here.
    // Returns pointer to new noteblock.
){
    struct noteblock* pNoteblock = allocate_noteblock ();
    if (pNoteblock == NULL) { return NULL; }
    pNoteblock->pNext = NULL;
    char* pText = get_ptr_to_text (pNoteblock);
    draw_staff (pText, NOTEBLOCK_WIDTH, STD_STAFF_BITSTR);

    const char LAST_IDX = sizeof (KEY_SIGNATURE_ROWS) - 1;
    int dir = (bits01to16 & 0b100) ? -1 : 1; // Direction - loop backwards or forwards through KEY_SIGNATURE_ROWS
    int col = 0;
    for (int i = 0; i <= LAST_IDX; ++i) {
        int row = KEY_SIGNATURE_ROWS[(dir > 0) ? i : (LAST_IDX - i)];
        char charToDraw =
            (bits01to16 & (1 << row)) ? (
                (bits17to32 & (1 << row)) ? '~' : 'b'
            ) : (
                (bits17to32 & (1 << row)) ? '#' : 1
            );
        if (charToDraw != 1) {
            switch (col) {
                case 0: draw_row (pText, row, charToDraw, 1, 1, 1, 1); break;
                case 1: draw_row (pText, row, 1, charToDraw, 1, 1, 1); break;
                case 2: draw_row (pText, row, 1, 1, charToDraw, 1, 1); break;
                case 3: draw_row (pText, row, 1, 1, 1, charToDraw, 1); break;
                case 4: draw_row (pText, row, 1, 1, 1, 1, charToDraw); break;
            }
            col = (col + 1) % NOTEBLOCK_WIDTH;
        }
    }

    return pNoteblock;
}


// Width of each type of barline (starting with single barline of width 3)
const unsigned char BARLINE_NOTEBLOCK_WIDTHS[6] = { 3, 4, 5, 5, 4, 1 };

// Make a barline noteblock
struct noteblock* make_barline (
    unsigned char byte // Bits 1-8 of barline encoding. Bits 5-7 are relevant here.
    // Returns pointer to new noteblock.
){
    struct noteblock* pNoteblock = allocate_noteblock ();
    if (pNoteblock == NULL) { return NULL; }
    pNoteblock->pNext = NULL;
    char* pText = get_ptr_to_text (pNoteblock);
    int width = ((byte >> 4) >= 6) ? NOTEBLOCK_WIDTH : BARLINE_NOTEBLOCK_WIDTHS[byte >> 4];
    draw_staff (pText, width, STD_STAFF_BITSTR);

    for (int row = ROW_LO_E; row <= ROW_HI_F; ++row) {
        draw_barline_row (pText, row, byte);
    }

    return pNoteblock;
}


// CLEF_TEXT constants - the full 80-char (16*5) text of each clef noteblock
const char     CLEF_TEXT_TREBLE[80] = "        _   / \\--|-/  |/ --|-- /|  /-|_-|/| \\|\\|-|\\_|_/--|--O_/                 ";
const char       CLEF_TEXT_BASS[80] = "               -__--/  \\0O--|-   /0--/-- /   /----     -----                    ";
const char CLEF_TEXT_PERCUSSION[80] = "               -----     ----- # # -#-#- # # -----     -----                    ";
const char      CLEF_TEXT_ERROR[80] = "ERRORE  O  R  R  R  E  O  R  R  R  E  O  R  R  R  E  O  R  R  R  E  O  R  RERROR";

// Make a clef noteblock
struct noteblock* make_clef (
    unsigned char byte // Bits 1-8 of clef encoding. Bits 7-8 are relevant here.
    // Returns pointer to new noteblock.
){
    struct noteblock* pNoteblock = allocate_noteblock ();
    if (pNoteblock == NULL) { return NULL; }
    pNoteblock->pNext = NULL;
    char* pText = get_ptr_to_text (pNoteblock);

    const char* clefText =
        (byte == 0b00100000) ? CLEF_TEXT_TREBLE :
        (byte == 0b01100000) ? CLEF_TEXT_BASS :
        (byte == 0b10100000) ? CLEF_TEXT_PERCUSSION : CLEF_TEXT_ERROR;
    for (int row = 0; row < NOTEBLOCK_HEIGHT; ++row) {
        for (int col = 0; col < NOTEBLOCK_WIDTH; ++col) {
            *(pText + (row * NOTEBLOCK_WIDTH) + col) = clefText[NOTEBLOCK_WIDTH * (NOTEBLOCK_HEIGHT - row - 1) + col];
        }
    }

    return pNoteblock;
}

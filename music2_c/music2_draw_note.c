//*************************************************************************************************
// music2_draw_note.c
// This file contains logic for drawing notation from one byte group type, note.
// This includes notes with different pitches, durations, and ornaments. This also includes rests.
//*************************************************************************************************


// External inclusions
#include <stddef.h> // NULL

// Internal inclusions
#include "music2_general1.h"
#include "music2_noteblock.h"


//*******************************************************************************
// Functions (mostly one-line) to get information about a note from a byte group
//*******************************************************************************

// Returns row (1-15) notehead should be on, or 0 if note is a rest
// (in which case no part of the rest should affect the 0th row).
inline int notehead_row (
    unsigned char byte1 // Bits 1-8 of note encoding. Bits 3-6 are relevant here.
){
    return (byte1 & 0b111100) >> 2;
}


// Whether a note-type noteblock is a rest
inline int is_rest (
    unsigned char byte1 // Bits 1-8 of note encoding. Bits 3-6 are relevant here.
){
    return notehead_row (byte1) == 0;
}


// Whether a note-type noteblock is a double whole note (AKA breve)
inline int note_is_double_whole (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
){
    return (byte2 & 0b1111) == 2;
}


// Whether a note-type noteblock is a whole note
inline int note_is_whole (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
){
    return (byte2 & 0b1111) == 3;
}


// Whether a note-type noteblock has a stem
inline int note_has_stem (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
){
    return (byte2 & 0b1111) >= 4;
}


// Whether a note-type noteblock's notehead is filled
inline int notehead_is_filled (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
){
    return (byte2 & 0b1111) >= 5;
}


// Whether a note-type noteblock has beams on the stem (on either or both sides)
inline int note_has_beams (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
){
    return (byte2 & 0b1111) >= 8;
}


// Whether a note-type noteblock has beams on the right side of the stem
inline int note_has_beams_right (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
){
    return (byte2 & 0b1111) >= 8 && (byte2 & 0b1111) < 12;
}


// A note-type noteblock's number of flags on the stem
inline int count_note_flags (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
){
    return ((byte2 & 0b1111) == 6) ? 1 : ((byte2 & 0b1111) == 7) ? 2 : 0;
}


// A note-type noteblock's number of beams on the stem (on either or both sides).
// Beamed eighth notes have one, beamed sixteenth notes have two, and others have zero.
inline int count_note_beams (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    // Returns number of beams a note has. Beamed eighth notes have one; beamed sixteenth notes have two.
){
    return note_has_beams (byte2) ? (1 + (byte2 & 0b1)) : 0;
}


// A note-type noteblock's number of beams on the right side of the stem.
inline int count_note_beams_right (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    // Returns number of right-side beams a note has.
){
    return note_has_beams_right (byte2) ? (1 + (byte2 & 0b1)) : 0;
}


// Whether a note-type noteblock with a beam has an extra-long stem
inline int note_has_long_stem (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
){
    return (0b1100110000000000 >> (byte2 & 0b1111)) & 0b1;
}


// Returns 1 or -1. If 1, the stem (if it exists) is on top and the articulation (if it exists) is on the bottom.
inline int note_orientation (
    unsigned char byte1 // Bits 1-8 of note encoding. Bits 3-6 are relevant here.
    // Returns 1 or -1. If 1, the stem (if it exists) is on top and the articulation (if it exists) is on the bottom.
){
    return (notehead_row (byte1) <= ROW_MD_B) ? 1 : -1;
}


// Whether a note-type noteblock's notehead is dotted
inline int notehead_is_dotted (
    unsigned char byte2 // Bits 9-16 of note encoding. Bit 13 is relevant here.
){
    return (byte2 & 0b10000) > 0;
}


// Whether a note-type noteblock's notehead is tied to the next note
inline int notehead_is_tied_to_next (
    unsigned char byte2 // Bits 9-16 of note encoding. Bit 14 is relevant here.
){
    return (byte2 & 0b100000) > 0;
}


// Accidental characters that go in front of the notehead - existing/none, flat, natural, sharp
const char ACCIDENTAL_CHARS[] = { 1, 'b', '~', '#' };

// Get the character to use to the left of the notehead
inline char pre_notehead_character (
    unsigned char byte1,   // Bits 1-8 of note encoding. Bits 3-8 are relevant here.
    int           prevTied // Whether the previous note is tied to this one.
    // Returns the character to use left of the notehead, or ASCII character 1 if existing
    // character should be used.
){
    return prevTied ? '_' : ACCIDENTAL_CHARS[byte1 >> 6]; // Index is bits 7-8
}


// Articulation characters that go over/under the notehead - existing/none, staccato, accent, tenuto
const char ARTICULATION_CHARS[] = { 1, '.', '>', '=' };

// Get the character to use above/below the notehead
inline char articulation_notehead_character (
    unsigned char byte2  // Bits 9-16 of note encoding. Bits 15-16 are relevant here.
    // Returns the character to use above/below the notehead, or ASCII character 1 if existing
    // character should be used.
){
    return ARTICULATION_CHARS[byte2 >> 6]; // Index is bits 15-16
}


// Get the character to use to the right of the notehead.
inline char post_notehead_character (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 13-14 are relevant here.
    // Returns the character to use left of the notehead, or ASCII character 1 if existing
    // character should be used.
){
    return notehead_is_dotted (byte2) ? '.' : notehead_is_tied_to_next (byte2) ? '_' : 1;
}


// Get the height of a note's stem
int stem_height (
    unsigned char byte1, // Bits 1-8 of note encoding. Bits 3-6 are relevant here.
    unsigned char byte2  // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    // Returns the number of rows the stem itself occupies, not including the notehead, and not including a possible
    // additional row used by a beam if such beam exists and the note has a positive orientation.
){
    if (!note_has_stem (byte2)) {
        return 0; // Whole or double-whole
    }
    if (!note_has_beams (byte2)) {
        return 2; // Quarter, flagged eighth, or flagged sixteenth
    }
    // Beamed eighth or beamed sixteenth:
    int noteheadOnSpace = row_is_space (notehead_row (byte1));
    int orientation = note_orientation (byte1);
    int hasLongStem = note_has_long_stem (byte2);
    return (noteheadOnSpace ? 3 : 2) + (orientation < 0) + (hasLongStem ? 2 : 0);
}



//******************
// Music note staff
//******************

// Get the staff bitstring for a note. Other noteblock types should always use STD_STAFF_BITSTR.
// Format: 1 represents a line row, 0 represents a space row.
// Most significant bit (left) represents ROW_HI_B, least significant bit (right) represents ROW_TEXT.
unsigned short staff_bitstr_for_note (
    unsigned char byte1 // Bits 1-8 of note encoding. Bits 3-6 are relevant here.
    // Returns the staff bitstring for the note.
){
    unsigned short staffBitstr = STD_STAFF_BITSTR;
    int noteheadRow = notehead_row (byte1);
    if (noteheadRow <= ROW_LO_C && noteheadRow != ROW_TEXT) {
        staffBitstr |= 0b0000000000000100;
    }
    else if (ROW_HI_A <= noteheadRow) {
        staffBitstr |= 0b0100000000000000;
    }
    return staffBitstr;
}



//*************************
// Make and draw functions
//*************************

// Draw a rest in a noteblock
void draw_rest (
    char*         pText, // (Pointer to) a noteblock's 2D array of text, in which to draw the rest
    unsigned char byte2  // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
){
    char d = notehead_is_dotted (byte2) ? '.' : 1;
    switch (byte2 & 0b1111) {
        case 0b0000: case 0b0001: // 0 permanently invalid, 1 currently unused
            draw_row_error (pText, ROW_MD_B);
            break;
        case 0b0010: // Double whole rest
            draw_row (pText, ROW_HI_D, 1, '#', '#', '#', 1);
            draw_row (pText, ROW_HI_C, 1, '#', '#', '#', d);
            draw_row (pText, ROW_MD_B, 1, '#', '#', '#', 1);
            break;
        case 0b0011: // Whole rest
            draw_row (pText, ROW_HI_D, 1, '#', '#', '#', 1);
            draw_row (pText, ROW_HI_C, 1, '#', '#', '#', d);
            break;
        case 0b0100: // Half rest
            draw_row (pText, ROW_HI_C, 1, '#', '#', '#', d);
            draw_row (pText, ROW_MD_B, 1, '#', '#', '#', 1);
            break;
        case 0b0101: // Quarter rest
            draw_row (pText, ROW_HI_D, 1, 1, '\\', 1, 1);
            draw_row (pText, ROW_HI_C, 1, 1, '/', d, 1);
            draw_row (pText, ROW_MD_B, 1, 1, '\\', 1, 1);
            draw_row (pText, ROW_LO_A, 1, 1, 'C', 1, 1);
            break;
        default: { // Eighth/sixteenth rests
            if ((byte2 & 0b1) == 0) { // Even - eighth rest
                draw_row (pText, ROW_HI_C, 1, 1, 1, 'O', d);
                draw_row (pText, ROW_MD_B, 1, 1, '/', 1, 1);
            }
            else { // Odd - sixteenth rest
                draw_row (pText, ROW_HI_C, 1, 1, 1, 'O', d);
                draw_row (pText, ROW_MD_B, 1, 1, 'O', 1, 1);
                draw_row (pText, ROW_LO_A, 1, '/', 1, 1, 1);
            }
        }
    }
}


// Draw the articulation, if one exists, below or above the notehead
void draw_articulation (
    char*         pText, // (Pointer to) a noteblock's 2D array text, in which to draw the articulation
    unsigned char byte1, // Bits 1-8 of note encoding
    unsigned char byte2  // Bits 9-16 of note encoding
){
    char articulationChar = articulation_notehead_character (byte2);
    if (articulationChar == 1) { return; }
    int noteheadRow = notehead_row (byte1);
    int orientation = note_orientation (byte1);
    int articulationRow = noteheadRow - orientation;
    if (articulationRow < ROW_TEXT || ROW_HI_B < articulationRow) { return; }
    draw_row (pText, articulationRow, 1, 1, articulationChar, 1, 1);
}


// Draws a 3-character notehead, as well as the characters to the left and right, for 5 total.
void draw_notehead (
    char*         pText,    // (Pointer to) a noteblock's 2D array text, in which to draw the notehead.
    unsigned char byte1,    // Bits 1-8 of note encoding.
    unsigned char byte2,    // Bits 9-16 of note encoding.
    unsigned int  parseInfo // Info stored between calls to parse_byte_group - see update_parse_info.
){
    char preNoteheadChar, leftChar, fillChar, rightChar, postNoteheadChar; // 5 characters to draw

    unsigned char prevByte2ForTie = (parseInfo & 0x00FF0000) >> 16;
    int prevTied = notehead_is_tied_to_next (prevByte2ForTie);
    preNoteheadChar = pre_notehead_character (byte1, prevTied); // 'b', '~', '#', or existing

    int noteheadRow = notehead_row (byte1); // Row to draw in

    if (note_has_stem (byte2)) {
        int orientation = note_orientation (byte1);
        leftChar = (orientation > 0) ? '(' : '|';
        fillChar = notehead_is_filled (byte2) ? '@' : '_';
        rightChar = (orientation > 0) ? '|' : ')';
    }
    else if (note_is_whole (byte2)) {
        leftChar = '('; fillChar = '_'; rightChar = ')';
    }
    else if (note_is_double_whole (byte2)) {
        leftChar = '|'; fillChar = 'O'; rightChar = '|';
    }
    else { // Invalid
        draw_row_error (pText, noteheadRow);
        return;
    }

    postNoteheadChar = post_notehead_character (byte2); // '.', '_', or existing

    draw_row (pText, noteheadRow, preNoteheadChar, leftChar, fillChar, rightChar, postNoteheadChar);
}


// Draw a note's stem, if it has one
void draw_stem (
    char* pText,       // (Pointer to) a noteblock's 2D array text, in which to draw.
    int   noteheadRow, // Row that the notehead is on. The bottom of the stem starts on an adjacent row.
    int   stemTopRow,  // Row that the top of the stem is on.
    int   orientation  // Note orientation. +1 means the stem is above the notehead. -1 means it's below.
){
    char c2 = (orientation > 0) ? 1 : '|';
    char c4 = (orientation > 0) ? '|' : 1;
    for (int row = stemTopRow; row != noteheadRow; row -= orientation) {
        draw_row (pText, row, 1, c2, 1, c4, 1);
    }
}


// Draw a note's flags, if it has any
void draw_flags (
    char* pText,       // (Pointer to) a noteblock's 2D array text, in which to draw.
    int   countFlags,  // Number of flags to draw, 0-2.
    int   stemTopRow,  // Row that the top of the stem is on. If there are two flags, this function will
          // draw one on this row and the other on the adjacent row closer to the notehead.
    int   orientation  // Note orientation. +1 means the stem is above the notehead. -1 means it's below.
){
    if (countFlags >= 1) {
        char char3 = (orientation > 0) ? 1 : '/';
        char char5 = (orientation > 0) ? '\\' : 1;
        draw_row (pText, stemTopRow, 1, 1, char3, 1, char5);
        if (countFlags == 2) {
            draw_row (pText, (stemTopRow - orientation), 1, 1, char3, 1, char5);
        }
    }
}


// Draw a note's beams, if it has any
void draw_beams (
    char* pText,           // (Pointer to) a noteblock's 2D array text, in which to draw.
    int   countLeftBeams,  // Number of beams to draw on the left of the stem, 0-2.
    int   countRightBeams, // Number of beams to draw on the right of the stem, 0-2.
    int   stemTopRow,      // Row that the top of the stem is on. If there are two beams, this function will
          // draw one on this row and the other on the adjacent row closer to the notehead.
    int   orientation      // Note orientation. +1 means the stem is above the notehead. -1 means it's below.
){
    if (countLeftBeams == 0 && countRightBeams == 0) {
        return; // If, for example, the user accidentally has a non-beamed note followed by a left-beamed note
    }
    if (orientation > 0) {
        // Left beam(s)
        if (countLeftBeams >= 1) {
            draw_row (pText, (stemTopRow + 1), '_', '_', '_', 1, 1);
            if (countLeftBeams == 2) {
                draw_row (pText, stemTopRow, '_', '_', '_', 1, 1);
            }
        }
        // Possible middle beam char above stem
        if (countLeftBeams >= 1 && countRightBeams >= 1) {
            draw_row (pText, (stemTopRow + 1), 1, 1, 1, '_', 1);
        }
        // Right beams
        if (countRightBeams >= 1) {
            draw_row (pText, (stemTopRow + 1), 1, 1, 1, 1, '_');
            if (countRightBeams == 2) {
                draw_row (pText, stemTopRow, 1, 1, 1, 1, '_');
            }
        }
    }
    else {
        // Left beam(s)
        if (countLeftBeams >= 1) {
            draw_row (pText, stemTopRow, '_', 1, 1, 1, 1);
            if (countLeftBeams == 2) {
                draw_row (pText, (stemTopRow + 1), '_', 1, 1, 1, 1);
            }
        }
        // Right beam(s)
        if (countRightBeams >= 1) {
            draw_row (pText, stemTopRow, 1, 1, '_', '_', '_');
            if (countRightBeams == 2) {
                draw_row (pText, (stemTopRow + 1), 1, 1, '_', '_', '_');
            }
        }
    }
}


// Draws a note's stem, if it has one, and flag(s) or beam(s), if it has them.
void draw_stem_flags_beams (
    char*         pText,    // (Pointer to) a noteblock's 2D array text, in which to draw.
    unsigned char byte1,    // Bits 1-8 of note encoding.
    unsigned char byte2,    // Bits 9-16 of note encoding.
    unsigned int  parseInfo // How many beams (0-2) this note should have on the left, if beamed.
){
    int stemHeight = stem_height (byte1, byte2);
    if (stemHeight == 0) { return; }
    int noteheadRow = notehead_row (byte1);
    int orientation = note_orientation (byte1);
    int countFlags = count_note_flags (byte2);
    int stemTopRow = noteheadRow + (orientation * stemHeight); // "top" meaning farthest from notehead

    draw_stem (pText, noteheadRow, stemTopRow, orientation);
    draw_flags (pText, countFlags, stemTopRow, orientation);

    // Draw beams, if any
    if (note_has_beams (byte2)) {
        unsigned char prevByte2ForBeams = (parseInfo & 0x0000FF00) >> 8;
        int countLeftBeams = count_note_beams_right (prevByte2ForBeams);
        int countRightBeams = count_note_beams_right (byte2);
        draw_beams (pText, countLeftBeams, countRightBeams, stemTopRow, orientation);
    }
}


// Make a note-type noteblock
struct noteblock* make_note (
    unsigned char byte1,    // Bits 1-8 of note encoding. Bits 1-2 should be 00.
    unsigned char byte2,    // Bits 9-16 of note encoding.
    unsigned int  parseInfo // Info stored between calls to parse_byte_group - see update_parse_info.
    // Returns pointer to new noteblock.
){
    struct noteblock* pNoteblock = allocate_noteblock ();
    if (pNoteblock == NULL) { return NULL; }
    pNoteblock->pNext = NULL;
    char* pText = get_ptr_to_text (pNoteblock);
    unsigned short staffBitstr = staff_bitstr_for_note (byte1);
    draw_staff (pText, NOTEBLOCK_WIDTH, staffBitstr);

    if (is_rest (byte1)) {
        draw_rest (pText, byte2);
    }
    else {
        draw_articulation (pText, byte1, byte2);
        draw_notehead (pText, byte1, byte2, parseInfo);
        draw_stem_flags_beams (pText, byte1, byte2, parseInfo);
    }

    return pNoteblock;
}

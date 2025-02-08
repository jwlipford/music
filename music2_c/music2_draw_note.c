//*************************************************************************************************
// music2_draw_note.c
// This file contains logic for drawing notation from two byte group types:
//   (Abbreviated NN) - Note or Rest, Not Beamed.
//   (Abbreviated NB) - Note, Beamed.
// Notes and rests have different pitches, durations, and ornaments.
//*************************************************************************************************


// External inclusions
#include <stddef.h> // NULL

// Internal inclusions
#include "music2_general1.h"
#include "music2_noteblock.h"


//*******************************************************************
// n_* functions to get information from a byte group of either type
//*******************************************************************

// Given the first byte of a byte group known to be either an NN note or NB note,
// returns 0 for NN or 1 for NB.
inline int n_isNB (
    unsigned char byte1 // Bits 1-8 of note encoding.
){
    return (byte1 & 0b0100) == 0b0100;
}

// Returns row (1-15) notehead should be on, or 0.
// For NN, 0 represent a rests. For NB, 0 is invalid.
inline int n_notehead_row (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
){
    return byte2 & 0b1111;
}


// Whether a note is followed by a tie/slur.
inline int n_is_tied (
    unsigned char byte1 // Bits 1-8 of note encoding. Bit 8 is relevant here.
){
    return (byte1 & 0b10000000) > 0;
}


// Whether a note is dotted.
inline int n_is_dotted (
    unsigned char byte2 // Bits 9-16 of note encoding. Bit 16 is relevant here.
){
    return (byte2 & 0b10000000) > 0;
}


// Accidental characters - Existing/none, flat, natural, sharp, tie/slur
const char ACCIDENTAL_CHARS[] = { 1, 'b', '~', '#', '_' };

// Get the character to use to the left of the notehead, or ASCII character 1 to not overwrite
// current character.
inline char n_pre_notehead_character (
    unsigned char byte1,   // Bits 1-8 of note encoding. Bits 4-5 are relevant here.
    int           prevTied // Whether the previous note is tied/slurred to this one.
){
    int index1 = (byte1 & 0b00011000) >> 3;
    int index2 = ((index1 == 0) && prevTied) ? 4 : index1;
    return ACCIDENTAL_CHARS[index2];
}


// Articulation characters - Existing/none, staccato, accent, tenuto
const char ARTICULATION_CHARS[] = { 1, '.', '>', '=' };

// Get the character to use above/below the notehead, or ASCII character 1 to not overwrite
// current character.
inline char n_articulation_notehead_character (
    unsigned char byte1 // Bits 1-8 of note encoding. Bits 6-7 are relevant here.
){
    int index = (byte1 & 0b01100000) >> 5;
    return ARTICULATION_CHARS[index];
}


// Get the character to use to the right of the notehead, or ASCII character 1 to not overwrite
// current character.
inline char n_post_notehead_character (
    unsigned char byte1, // Bits 1-8 of note encoding. Bit 8 is relevant here.
    unsigned char byte2  // Bits 9-16 of note encoding. Bits 16 is relevant here.
){
    return n_is_dotted (byte2) ? '.' : n_is_tied (byte1) ? '_' : 1;
}



//********************************************************************************
// nn_* functions to get information from an NN "Note or Rest, Not Beamed" byte group
//********************************************************************************

// nn_DUR note duration constants (2-7 valid, 0-1 invalid)
#define nn_DUR_BREVE     (2)
#define nn_DUR_WHOLE     (3)
#define nn_DUR_HALF      (4)
#define nn_DUR_QUARTER   (5)
#define nn_DUR_EIGHTH    (6)
#define nn_DUR_SIXTEENTH (7)

// Get an NN note's duration.
inline int nn_duration (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 13-15 are relevant here.
){
    return (byte2 & 0b01110000) >> 4;
}

// Whether an NN note has a stem.
inline int nn_has_stem (
    int duration // Duration from function nn_duration
){
    return duration >= nn_DUR_HALF;
}

// Whether an NN note's notehead is filled.
inline int nn_is_notehead_filled (
    int duration // Duration from function nn_duration
){
    return duration >= nn_DUR_QUARTER;
}

// An NN note's number of flags (0-2)
inline int nn_count_flags (
    int duration // Duration from function nn_duration
){
    return (duration <= nn_DUR_QUARTER) ? 0 : (duration - nn_DUR_QUARTER);
}

// Whether an NN byte group represents a rest.
inline int nn_is_rest (
    int row // Notehead row from function n_notehead_row
){
    return row == 0;
}

// Returns 1 or -1. If 1, the NN note's stem (if it exists) is on top and the articulation (if it exists)
// is on the bottom, otherwise the opposite.
inline int nn_orientation (
    int noteheadRow // Notehead row (1-15) from function n_notehead_row
){
    // Same as (ROW_MD_B < noteheadRow) ? -1 : 1
    return 1 - ((ROW_MD_B < noteheadRow) * 2);
}



//************************************************************************
// nb_* functions to get information from an NB "Note, Beamed" byte group
//************************************************************************

// Whether NB nb note's notehead row is valid
inline int nb_is_notehead_row_valid (
    int row // Notehead row (0-15) from function n_notehead_row
){
    return row > 0;
}

// Get an NB note's stem length in range 1-4
inline int nb_stem_length (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 13-14 are relevant here.
){
    return ((byte2 & 0b00110000) >> 4) + 1;
}

// Returns 1 or -1. If 1, the NB note's stem (if it exists) is on top and the articulation (if it exists)
// is on the bottom, otherwise the opposite.
inline int nb_orientation (
    unsigned char byte2 // Bits 9-16 of note encoding. Bit 15 is relevant here.
){
    // Same as ((byte2 & 0b01000000) >> 6) ? 1 : -1
    return 1 - (((byte2 & 0b01000000) >> 6) * 2);
}

// Get an NB note's number of beams on the left side of the stem
inline int nb_beam_count_left (
    unsigned char byte3 // Bits 17-24 of note encoding. Bits 19-20 are relevant here.
){
    return (byte3 & 0b1100) >> 2;
}

// Get an NB note's number of beams on the right side of the stem
inline int nb_beam_count_right (
    unsigned char byte3 // Bits 17-24 of note encoding. Bits 17-18 are relevant here.
){
    return byte3 & 0b11;
}

// Get an NB note's number of narrow 2-character beams.
// They are on either the left or the right, depending on the note orientation.
// The remaining beams on that side are wide 3-character beams.
inline int nb_beam_count_narrow (
    unsigned char byte3 // Bits 17-24 of note encoding. Bits 21-22 are relevant here.
){
    return (byte3 & 0b110000) >> 4;
}

// A basic check for whether the last two bits of an NB byte group's byte 3 are valid
inline int nb_are_bits_23_24_valid (
    unsigned char byte3 // Bits 17-24 of note encoding. Bits 23-24 are relevant here.
){
    return (byte3 & 0b11000000) == 0b11000000;
}



//******************
// Music note staff
//******************

// Get the staff bitstring for an NN or NB note. Other byte group types should always use STD_STAFF_BITSTR.
// Format: 1 represents a line row, 0 represents a space row.
// Most significant bit (left) represents ROW_HI_B, least significant bit (right) represents ROW_TEXT.
unsigned short n_staff_bitstr_for_note (
    unsigned char byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    // Returns the staff bitstring for the note.
){
    unsigned short staffBitstr = STD_STAFF_BITSTR;
    int noteheadRow = n_notehead_row (byte2);
    if (noteheadRow <= ROW_LO_C && noteheadRow != ROW_TEXT) {
        staffBitstr |= 0b0000000000000100;
    }
    else if (ROW_HI_A <= noteheadRow) {
        staffBitstr |= 0b0100000000000000;
    }
    return staffBitstr;
}



//********************
// n_* draw functions
//********************

// Draw an NN or NB notes's pre-notehead character (in column 1 - accidental or nothing)
// and post-notehead character (in column 5 - dot, tie, or nothing).
void n_draw_pre_post_notehead_chars (
    char*         pText,    // (Pointer to) a noteblock's 2D array text, in which to draw the notehead.
    unsigned char byte1,    // Bits 1-8 of note encoding.
    unsigned char byte2,    // Bits 9-16 of note encoding.
    unsigned int  parseInfo // Info stored between calls to parse_byte_group - see update_parse_info.
){
    char preNoteheadChar, postNoteheadChar; // 2 chars to draw
    unsigned char prevByte1 = (parseInfo & 0xFF00) >> 8;
    int prevTied = n_is_tied (prevByte1);
    preNoteheadChar = n_pre_notehead_character (byte1, prevTied);
    postNoteheadChar = n_post_notehead_character (byte1, byte2);
    int noteheadRow = n_notehead_row (byte2);
    draw_row (pText, noteheadRow, preNoteheadChar, 1, 1, 1, postNoteheadChar);
}


// Draw an NN or NB note's articulation, if one exists, below or above the notehead
void n_draw_articulation (
    char*         pText, // (Pointer to) a noteblock's 2D array text, in which to draw the articulation
    unsigned char byte1, // Bits 1-8 of note encoding
    unsigned char byte2  // Bits 9-16 of note encoding
){
    char articulationChar = n_articulation_notehead_character (byte1);
    if (articulationChar == 1) { return; }
    int isNB = n_isNB (byte1);
    int noteheadRow = n_notehead_row (byte2);
    int orientation = isNB ? nb_orientation (byte2) : nn_orientation (noteheadRow);
    int articulationRow = noteheadRow - orientation;
    if (articulationRow < ROW_TEXT || ROW_HI_B < articulationRow) { return; }
    draw_row (pText, articulationRow, 1, 1, articulationChar, 1, 1);
}



//*********************
// nn_* draw functions
//*********************

// Draw a rest in a noteblock
void nn_draw_rest (
    char*         pText, // (Pointer to) a noteblock's 2D array of text, in which to draw.
    unsigned char byte2  // Bits 9-16 of note encoding.
){
    int duration = nn_duration (byte2);
    char d = n_is_dotted (byte2) ? '.' : 1;

    if (duration == nn_DUR_BREVE) {
        draw_row (pText, ROW_HI_D, 1, '#', '#', '#', 1);
        draw_row (pText, ROW_HI_C, 1, '#', '#', '#', d);
        draw_row (pText, ROW_MD_B, 1, '#', '#', '#', 1);
    }
    else if (duration == nn_DUR_WHOLE) {
        draw_row (pText, ROW_HI_D, 1, '#', '#', '#', 1);
        draw_row (pText, ROW_HI_C, 1, '#', '#', '#', d);
    }
    else if (duration == nn_DUR_HALF) {
        draw_row (pText, ROW_HI_C, 1, '#', '#', '#', d);
        draw_row (pText, ROW_MD_B, 1, '#', '#', '#', 1);
    }
    else if (duration == nn_DUR_QUARTER) {
        draw_row (pText, ROW_HI_D, 1, 1, '\\', 1, 1);
        draw_row (pText, ROW_HI_C, 1, 1, '/', d, 1);
        draw_row (pText, ROW_MD_B, 1, 1, '\\', 1, 1);
        draw_row (pText, ROW_LO_A, 1, 1, 'C', 1, 1);
    }
    else if (duration == nn_DUR_EIGHTH) {
        draw_row (pText, ROW_HI_C, 1, 1, 1, 'O', d);
        draw_row (pText, ROW_MD_B, 1, 1, '/', 1, 1);
    }
    else if (duration == nn_DUR_SIXTEENTH) {
        draw_row (pText, ROW_HI_C, 1, 1, 1, 'O', d);
        draw_row (pText, ROW_MD_B, 1, 1, 'O', 1, 1);
        draw_row (pText, ROW_LO_A, 1, '/', 1, 1, 1);
    }
    else {
        draw_row_error (pText, ROW_MD_B);
    }
}


// Draws an NN note's 3-character notehead (in columns 2-4).
void nn_draw_notehead (
    char*         pText, // (Pointer to) a noteblock's 2D array text, in which to draw the notehead.
    unsigned char byte2  // Bits 9-16 of note encoding.
){
    char leftChar, fillChar, rightChar; // 3 characters to draw
    int noteheadRow = n_notehead_row (byte2);
    int duration = nn_duration (byte2);

    if (nn_has_stem (duration)) {
        int orientation = nn_orientation (noteheadRow);
        leftChar = (orientation > 0) ? '(' : '|';
        fillChar = nn_is_notehead_filled (duration) ? '@' : '_';
        rightChar = (orientation > 0) ? '|' : ')';
    }
    else if (duration == nn_DUR_WHOLE) {
        leftChar = '('; fillChar = '_'; rightChar = ')';
    }
    else if (duration == nn_DUR_BREVE) {
        leftChar = '|'; fillChar = 'O'; rightChar = '|';
    }
    else { // Invalid
        draw_row_error (pText, noteheadRow);
        return;
    }

    draw_row (pText, noteheadRow, 1, leftChar, fillChar, rightChar, 1);
}


// Draw an NN note's stem and flags, if it has them
void nn_draw_stem_flags (
    char* pText,        // (Pointer to) a noteblock's 2D array text, in which to draw.
    unsigned char byte2 // Bits 9-16 of note encoding.
){
    int duration = nn_duration (byte2);
    int hasStem = nn_has_stem (duration);
    if (!hasStem) return;
    int countFlags = nn_count_flags (duration);

    int noteheadRow = n_notehead_row (byte2);
    int orientation = nn_orientation (noteheadRow);

    if (orientation > 0) {
        char ff = (countFlags > 0) ? '\\' : 1;
        char fn = (countFlags > 1) ? '\\' : 1;
        draw_row (pText, (noteheadRow + 2), 1, 1, 1, '|', ff);
        draw_row (pText, (noteheadRow + 1), 1, 1, 1, '|', fn);
    }
    else {
        char fn = (countFlags > 1) ? '/' : 1;
        char ff = (countFlags > 0) ? '/' : 1;
        draw_row (pText, (noteheadRow - 1), 1, '|', fn, 1, 1);
        draw_row (pText, (noteheadRow - 2), 1, '|', ff, 1, 1);
    }
}



//*********************
// nb_* draw functions
//*********************

// Draws an NB note's 3-character notehead (in columns 2-4).
void nb_draw_notehead (
    char*         pText, // (Pointer to) a noteblock's 2D array text, in which to draw.
    unsigned char byte2  // Bits 9-16 of note encoding.
){
    int noteheadRow = n_notehead_row (byte2);
    int orientation = nb_orientation (byte2);
    char leftChar = (orientation > 0) ? '(' : '|';
    char rightChar = (orientation > 0) ? '|' : ')';
    draw_row (pText, noteheadRow, 1, leftChar, '@', rightChar, 1);
}


// Draw an NB note's stem and beams (if any).
void nb_draw_stem_beams (char* pText, unsigned char byte2, unsigned char byte3) {
    // Get info from bytes. Beam count vars are mutable and may decrease as rows are drawn.
    int noteheadRow = n_notehead_row (byte2);
    int stemLength = nb_stem_length (byte2);
    int orientation = nb_orientation (byte2);
    int beamCountLeft = nb_beam_count_left (byte3);
    int beamCountRight = nb_beam_count_right (byte3);
    int beamCountNarrow = nb_beam_count_narrow (byte3);
    // If orientation is up and there are beams, will draw additonal beam row above top of stem
    int beamCountAboveStem = (orientation > 0) && (beamCountLeft || beamCountRight);
    // Start with farthest row from notehead
    int row = noteheadRow + (stemLength * orientation) + beamCountAboveStem;
    int rowOk = (ROW_LO_B <= row) && (row <= ROW_HI_B);
    if (!rowOk) {
        row = noteheadRow - orientation; // If actual row was invalid then this is valid
        draw_row_error (pText, row);
        return;
    }
    // Beam count wide + beam count narrow = beam count on the side with the distinction
    int beamCountWide = ((orientation > 0) ? beamCountLeft : beamCountRight) - beamCountNarrow;
    int beamsOk = (beamCountWide > 0 || beamCountNarrow == 0)
        && (beamCountLeft <= (stemLength + beamCountAboveStem))
        && (beamCountRight <= (stemLength + beamCountAboveStem));
    if (!beamsOk)
    {
        draw_row_error (pText, row);
        return;
    }
    // cW = wide beam char, cL = left beam char, cS = char in stem column, cR = right beam char
    char cW = '_', cL = '_', cR = '_';
    char cS = (beamCountAboveStem && beamCountLeft && beamCountRight) ? '_' : 1;
    // Draw normal rows from farthest to nearest
    while (row != noteheadRow) {
        if (beamCountLeft == 0) { cL = 1; }
        if (beamCountRight == 0) { cR = 1; }
        if (beamCountWide == 0) { cW = 1; }
        if (beamCountAboveStem == 0) { cS = '|'; }
        if (orientation > 0) { draw_row (pText, row, cW, cL, cL, cS, cR); }
        if (orientation < 0) { draw_row (pText, row, cL, cS, cR, cR, cW); }
        if (beamCountLeft > 0) { --beamCountLeft; }
        if (beamCountRight > 0) { --beamCountRight; }
        if (beamCountWide > 0) { --beamCountWide; }
        if (beamCountAboveStem > 0) { --beamCountAboveStem; }
        row -= orientation;
    }
}



//**************************
// Make noteblock functions
//**************************

// Make a "Note or Rest, Not Beamed" noteblock
struct noteblock* make_nn (
    unsigned char byte1,    // Bits 1-8 of note encoding. Bits 1-3 should be 001.
    unsigned char byte2,    // Bits 9-16 of note encoding.
    unsigned int  parseInfo // Info stored between calls to parse_byte_group - see update_parse_info.
    // Returns pointer to new noteblock.
){
    struct noteblock* pNoteblock = allocate_noteblock ();
    if (pNoteblock == NULL) { return NULL; }
    pNoteblock->pNext = NULL;
    char* pText = get_ptr_to_text (pNoteblock);
    unsigned short staffBitstr = n_staff_bitstr_for_note (byte2);
    draw_staff (pText, NOTEBLOCK_WIDTH, staffBitstr);

    int noteheadRow = n_notehead_row (byte2);
    if (nn_is_rest (noteheadRow)) {
        nn_draw_rest (pText, byte2);
    }
    else {
        nn_draw_notehead (pText, byte2);
        n_draw_pre_post_notehead_chars (pText, byte1, byte2, parseInfo);
        n_draw_articulation (pText, byte1, byte2);
        nn_draw_stem_flags (pText, byte2);
    }

    return pNoteblock;
}


// Make a "Note, Beamed" noteblock
struct noteblock* make_nb (
    unsigned char byte1,    // Bits 1-8 of note encoding. Bits 1-3 should be 101.
    unsigned char byte2,    // Bits 9-16 of note encoding.
    unsigned char byte3,    // Bits 17-24 of note encoding.
    unsigned int  parseInfo // Info stored between calls to parse_byte_group - see update_parse_info.
    // Returns pointer to new noteblock.
){
    struct noteblock* pNoteblock = allocate_noteblock ();
    if (pNoteblock == NULL) { return NULL; }
    pNoteblock->pNext = NULL;
    char* pText = get_ptr_to_text (pNoteblock);
    unsigned short staffBitstr = n_staff_bitstr_for_note (byte2);
    draw_staff (pText, NOTEBLOCK_WIDTH, staffBitstr);

    int noteheadRow = n_notehead_row (byte2);
    if (!nb_is_notehead_row_valid (noteheadRow) || !nb_are_bits_23_24_valid (byte3)) {
        draw_row_error (pText, ROW_MD_B);
        return pNoteblock;
    }

    nb_draw_notehead (pText, byte2);
    n_draw_pre_post_notehead_chars (pText, byte1, byte2, parseInfo);
    n_draw_articulation (pText, byte1, byte2);
    nb_draw_stem_beams (pText, byte2, byte3);

    return pNoteblock;
}

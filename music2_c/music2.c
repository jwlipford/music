//*****************************************************************************************************
// music2.c
// Takes a bit string as input, converts to music notation which is printed.
// This is primarily an exercise in squeezing high information density out of a small number of bytes.
// This is version 2.
//*****************************************************************************************************


#include <limits.h> // UINT_MAX
#include <stdio.h>  // printf, fopen_s
#include <stdlib.h> // malloc, atoi
#include <stddef.h> // NULL
#include <string.h> // strcmp, strcpy
#include <time.h>   // time



//***********************
// Help text and example
//***********************

// Command line arguments
const char* STR_HELP =
"  music.exe prints sheet music (drawn with ASCII characters) from an encoded file. Options:\n"
"    music.exe -e                   Print file encoding information\n"
"    music.exe -v                   Print example of visual style of output\n"
"    music.exe <filepath>           Read a file and print music on a continuous staff\n"
"    music.exe <filepath> <width>   Read a file and print music with a maximum page width (min 5, max 255)\n"
"    music.exe -p <count>           Test performance by repeatedly constructing the example from option -v\n";

// File encoding
const char* STR_ENCODING =
"  FILE CREATION\n"
"  To create an encoded file, you can use the PowerShell set-content cmdlet. For example:\n"
"    $b=[byte[]]@(0x53,0x61,0x75,0x63,0x65)\n"
"    set-content encoded_song $b -encoding byte\n"
"  FILE ENCODING\n"
"  File structure:\n"
"    A file consists of 1 or more groups of 1-4 bytes. Most byte groups represent \"noteblocks.\"\n"
"    A noteblock is a rectangle of text, 16 characters high and 1-5 characters wide, that represents a clef,\n"
"    note, barline, or some other large element of music notation.\n"
"    Noteblocks are added to the end of the staff one by one.\n"
"    Additionally, a dynamics text byte group modifies the previous byte group's noteblock.\n"
"    Finally, a terminator byte is required at the end of the file.\n"
"  Invalid input:\n"
"    If you ever see a capital E or the string \"ERROR\" in the generated music, your file has invalid input.\n"
"  Bit notation:\n"
"    In the following byte group descriptions, we count bits from the right, the 1's place, starting at 1.\n"
"    For example, in the bit string 10010, the zeros are bits 1, 3, and 4; the ones are bits 2 and 5.\n"
"    For two bytes, their bits numbered in base 32 are: 87654321 GFEDCBA9.\n"
"  BYTE GROUP TYPES\n"
"  Terminator (1 byte):\n"
"    Bits 1-8: Always 00000000\n"
"  Note/Rest (2 bytes):\n"
"    Bits 1-2:   Always 01\n"
"    Bits 3-6:   Rest (0) or pitches low B (1) to middle B (8) to high B (15)\n"
"    Bits 7-8:   Accidentals - none (0), flat ('b') (1), natural ('~') (2), or sharp ('#') (3)\n"
"    Bits 9-12:\n"
"      Appearance - Invalid (0), unused (1), Breve (2), whole (3), half (4), quarter (5), eighth (6-14 even),\n"
"      sixteenth (7-15 odd). For eighths and sixteenths, there are five encodings each that indicate whether\n"
"      the note is flagged or beamed and, if beamed, the beam height:\n"
"        Flagged (6-7) VS beamed (8-15).\n"
"        Beamed on left only with number of left-side beams determined by previous note (12-15), VS\n"
"          beamed on right at least once with number of left-side beams determined by previous note (8-11).\n"
"        Beam two spaces away (8-9, 12-13) VS three spaces away (10-11, 14-15).\n"
"    Bit  13:    Dotted\n"
"    Bit  14:    Tie/slur after\n"
"    Bits 15-16: Articulations: None (0), staccato (1), accent (2), tenuto (3)\n"
"  Time change (1 byte):\n"
"    Bits 1-2: Always 10\n"
"    Bits 3-4: Bottom number - 1, 2, 4, or 8 (encoded as 0 to 3)\n"
"    Bits 5-8: Top number - 1 to 16 (encoded as 0 to 15)\n"
"  Key change (4 bytes):\n"
"    Bits 1-2: Always 11\n"
"    Bit  3:   Arrangement of accidentals - Resembling Db major scale (0) or B major scale (1)\n"
"    Bits 4-14, 20-30:\n"
"      Pitches go from low D (bits 4 and 20) to high G (bits 14 and 30).\n"
"      If a pitch has a 1 in the first bit string but not the second, it is flat ('b').\n"
"      If a pitch has a 1 in the second bit string but not the first, it is sharp ('#').\n"
"      If a pitch has a 1 in both bit strings, it is natural ('~').\n"
"    Bits 15-16, 17-19, 31-32:\n"
"      Use these bits to make sure each byte has at least one 1 so it's not a terminator.\n"
"  Barline (1 byte):\n"
"    Bits 1-4: Always 0100\n"
"    Bit  5-7: Type of barline -\n"
"              single (0), double (1), left repeat (2), right repeat (3), both repeats (4), blank column (5)\n"
"    Bit  8:   Unused\n"
"  Dynamics text (3 bytes):\n"
"    Applies to the previous noteblock. (Non-note noteblocks might need to have part of a crescendo/\n"
"    decrescendo under them.) Invalid if this is the first byte.\n"
"    Bits 1-4: Always 1000\n"
"    Bits 5-8, 9-12, 13-16, 17-20, 21-24:\n"
"      Each group of four bits represents one of these characters (encoded as 1-13, 0 invalid, 14-15 unused):\n"
"      Null, space, '<', '>', '.', 'c', 'd', 'e', 'f', 'm', 'p', 'r', 's'\n"
"      These 12 characters can make text such as \" ppp \", \"cresc\", \" decr\", \" mp<<\", \"<<f>>\", etc.\n"
"  Clef (1 byte):\n"
"    Elsewhere in these descriptions, pitch names assume treble clef, but here you can draw a different clef.\n"
"    Bits 1-6: Always 100000\n"
"    Bits 7-8: Type - Treble (0), bass (1), percussion (2)\n";

// Example song to illustrate visual style of output. Used by str_example function.
const unsigned char EXAMPLE_BYTES[] = {
    0b01010100, // Blank column
    0b00100000, // Treble clef
    0b00000111, 0b11000000, 0b00000111, 0b11110110, // E major (C# minor) key signature - C#, D#, F#, G#
    0b00111010, // 4|4 time signature
    0b00000001, 0b00000110, // Eighth rest
    0b00011001, 0b00001010, // Low G, beamed eighth (left and right, tall stem)
    0b00101000, 0b10111010, 0b00100010, // Dynamics text " mp  "
    0b00011101, 0b00011000, // Low A, beamed eighth (left and right), dotted
    0b00011001, 0b00011010, // Low G, beamed eighth (left and right, tall stem), dotted
    0b00011101, 0b00001100, // Low A, beamed eighth (left only)
    0b00010001, 0b00000101, // Low E, quarter
    0b00000100, // Single barline
    0b00000001, 0b00000110, // Eighth rest
    0b00011001, 0b00001010, // Low G, beamed eighth (left and right, tall stem)
    0b00011101, 0b00011000, // Low A, beamed eighth (left and right), dotted
    0b00011001, 0b00011010, // Low G, beamed eighth (left and right, tall stem), dotted
    0b00011101, 0b00001100, // Low A, beamed eighth (left only)
    0b00010001, 0b00000101, // Low E, quarter
    0b00000100, // Single barline
    0b00000001, 0b00000110, // Eighth rest
    0b00011001, 0b00001010, // Low G, beamed eighth (left and right, tall stem)
    0b00011101, 0b00011000, // Low A, beamed eighth (left and right), dotted
    0b00011001, 0b00011010, // Low G, beamed eighth (left and right, tall stem), dotted
    0b00011101, 0b00001100, // Low A, beamed eighth (left only)
    0b00010001, 0b00000101, // Low E, quarter
    0b00000100, // Single barline
    0b00000001, 0b00000110, // Eighth rest
    0b00011001, 0b00001010, // Low G, beamed eighth (left and right, tall stem)
    0b00101000, 0b00110010, 0b00110011, // Dynamics text "  <<<"
    0b00011101, 0b00011000, // Low A, beamed eighth (left and right), dotted
    0b00111000, 0b00110011, 0b00110011, // Dynamics text "<<<<<"
    0b00011001, 0b00011010, // Low G, beamed eighth (left and right, tall stem), dotted
    0b00111000, 0b00110011, 0b00110011, // Dynamics text "<<<<<"
    0b00011101, 0b00001100, // Low A, beamed eighth (left only)
    0b00111000, 0b00110011, 0b00110011, // Dynamics text "<<<<<"
    0b00010001, 0b00000110, // Low E, lone eighth
    0b00111000, 0b00110011, 0b00110011, // Dynamics text "<<<<<"
    0b00100101, 0b01001001, // High C, beamed sixteenth (left and right), staccato
    0b00111000, 0b10010010, 0b00010010, // Dynamics text "< f  "
    0b00100101, 0b01101101, // High C, beamed sixteenth (left only), tied to next, staccato
    0b00100100, // Left repeat barline :|
    0b00100101, 0b00100100, // High C, half, tied to nothing
    0b00000000  // Terminator
};

// The following width splits the string representation of the above bytes neatly into two staves.
#define EXAMPLE_WIDTH (85)



//********************************************************************
// Noteblock struct - data structure, constants, and simple functions
//********************************************************************

// Structure for text used by a note, time signature, key signature, barline, etc.
// Also has pointer to next noteblock in linked list of noteblocks

#define NOTEBLOCK_WIDTH   (5)
#define NOTEBLOCK_HEIGHT (16)

struct noteblock {
    struct noteblock* pNext;
    // Next noteblock
    char text[NOTEBLOCK_HEIGHT][NOTEBLOCK_WIDTH];
    // 2D array of text for the noteblock. Not a pointer.
    // If <5 characters wide, terminate with \0, but make sure each row is same length.
};


// ROW constants (some unused, but defining all)

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



//*******************************************************
// Small inline functions for interpreting encoded bytes
//*******************************************************

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



//********************************************************
// Larger inline functions for interpreting encoded bytes
//********************************************************

const char ACCIDENTAL_CHARS[] = { 1, 'b', '~', '#' }; // Existing/none, flat, natural, sharp

// Get the character to use to the left of the notehead
inline char pre_notehead_character (
    unsigned char byte1,   // Bits 1-8 of note encoding. Bits 3-8 are relevant here.
    int           prevTied // Whether the previous note is tied to this one.
    // Returns the character to use left of the notehead, or ASCII character 1 if existing
    // character should be used.
){
    return prevTied ? '_' : ACCIDENTAL_CHARS[byte1 >> 6]; // Index is bits 7-8
}


const char ARTICULATION_CHARS[] = { 1, '.', '>', '=' }; // Existing/none, staccato, accent, tenuto

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


//************************
// Other inline functions
//************************

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


//************************
// Larger misc. functions
//************************

// BYTE_GROUP_TYPE constants representing the type of a byte group (1-4 bytes from the encoded file).
// For fun, rather than using consecutive integers, I made each constant resemble the binary number(s)
// that represent it, but this doesn't drive functionality.

#define BYTE_GROUP_TYPE_TERMINATOR  (0b00000000)
#define BYTE_GROUP_TYPE_NOTE        (0b00000001)
#define BYTE_GROUP_TYPE_TIME_CHANGE (0b00000010)
#define BYTE_GROUP_TYPE_KEY_CHANGE  (0b00000011)
#define BYTE_GROUP_TYPE_BARLINE     (0b00000100)
#define BYTE_GROUP_TYPE_DYN_TEXT    (0b00001000)
#define BYTE_GROUP_TYPE_CLEF        (0b00100000)
#define BYTE_GROUP_TYPE_INVALID     (0b11010000)


// Get a byte group's type from the first byte in the group
int byte_group_type (
    unsigned char byte1 // Bits 1-8 of note encoding.
    // Returns one of the BYTE_GROUP_TYPE constants.
){
    // At this point, we know nothing about byte1
    switch (byte1 & 0b11) {
        case 0b01: return BYTE_GROUP_TYPE_NOTE;
        case 0b10: return BYTE_GROUP_TYPE_TIME_CHANGE;
        case 0b11: return BYTE_GROUP_TYPE_KEY_CHANGE;
    }
    // At this point, we know byte1 & 0b11 == 0b00
    switch (byte1 & 0b1100) {
        case 0b0100: return BYTE_GROUP_TYPE_BARLINE;
        case 0b1000: return BYTE_GROUP_TYPE_DYN_TEXT;
        case 0b1100: return BYTE_GROUP_TYPE_INVALID;
    }
    // At this point, we know byte1 & 0b1111 == 0b0000
    switch (byte1 & 0b110000) {
        case 0b100000: return BYTE_GROUP_TYPE_CLEF;
    }
    switch (byte1) {
        case 0: return BYTE_GROUP_TYPE_TERMINATOR;
    }
    return BYTE_GROUP_TYPE_INVALID;
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



//*****************************************************
// "Draw" functions to draw within existing noteblocks
//*****************************************************

// Draw the staff (spaces and lines) in a noteblock.
void draw_staff (
    char* pText,          // (Pointer to) a noteblock's 2D array of text, in which to draw the staff.
    int   width,          // Width of noteblock. If less than 5, remaining column(s) will be filled with '\0's.
    int   ledgerLineFlags // 1st bit: Whether bottom (low C) ledger line is needed.
                          // 2nd bit: Whether top (high A) ledger line is needed.
){
    char cs[NOTEBLOCK_WIDTH]; // Space characters - ' ' for columns < width, '\0' for columns >= width
    char cL[NOTEBLOCK_WIDTH]; // Line characters  - '-' for columns < width, '\0' for columns >= width
    for (int i = 0; i < NOTEBLOCK_WIDTH; ++i) {
        cs[i] = (i < width) ? ' ' : '\0';
        cL[i] = (i < width) ? '-' : '\0';
    }
    unsigned short areRowsLines = // Bit string of length 16. 1 means line, 0 means space. High B (left) to Text (right).
        (ledgerLineFlags == 0) ? 0b0001010101010000 :
        (ledgerLineFlags == 1) ? 0b0001010101010100 :
        (ledgerLineFlags == 2) ? 0b0101010101010000 :
                                 0b0101010101010100;
    for (int i = ROW_TEXT; i <= ROW_HI_B; ++i) {
        if ((areRowsLines >> i) & 0b1) {
            draw_row_raw (pText, i, cL[0], cL[1], cL[2], cL[3], cL[4]);
        }
        else {
            draw_row_raw (pText, i, cs[0], cs[1], cs[2], cs[3], cs[4]);
        }
    }
}


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
    char* pText,       // (Pointer to) a noteblock's 2D array text, in which to draw.
    int   countLeftBeams,  // Number of beams to draw on the left of the stem, 0-2.
    int   countRightBeams, // Number of beams to draw on the right of the stem, 0-2.
    int   stemTopRow,  // Row that the top of the stem is on. If there are two beams, this function will
                       // draw one on this row and the other on the adjacent row closer to the notehead.
    int   orientation  // Note orientation. +1 means the stem is above the notehead. -1 means it's below.
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
    char*         pText,         // (Pointer to) a noteblock's 2D array text, in which to draw.
    unsigned char byte1,         // Bits 1-8 of note encoding.
    unsigned char byte2,         // Bits 9-16 of note encoding.
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



//********************************************************
// "Make" functions to make different types of noteblocks
//********************************************************

// Make a note-type noteblock
struct noteblock* make_note (
    unsigned char byte1,    // Bits 1-8 of note encoding. Bits 1-2 should be 00.
    unsigned char byte2,    // Bits 9-16 of note encoding.
    unsigned int  parseInfo // Info stored between calls to parse_byte_group - see update_parse_info.
    // Returns pointer to new noteblock.
){
    struct noteblock* pNoteblock = malloc (sizeof (struct noteblock));
    if (pNoteblock == NULL) { return NULL; }
    pNoteblock->pNext = NULL;
    char* pText = get_ptr_to_text (pNoteblock);
    int noteheadRow = notehead_row (byte1);
    int ledgerLineFlags = // 1st bit for low ledger line, 2nd bit for high ledger line
        (noteheadRow <= ROW_LO_C && noteheadRow != ROW_TEXT) + ((ROW_HI_A <= noteheadRow) << 1);
    draw_staff (pText, NOTEBLOCK_WIDTH, ledgerLineFlags);

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


// Make a time signature noteblock
struct noteblock* make_time_signature (
    unsigned char byte // Bits 1-8 of time signature encoding. Bits 3-8 are relevant here.
    // Returns pointer to new noteblock.
){
    int topNum = (byte / 16) + 1; // In range 1-16
    int btmNum = 1 << ((byte / 4) % 4); // In {1,2,4,8}
    int topIs2Digits = (topNum > 9);

    struct noteblock* pNoteblock = malloc (sizeof (struct noteblock));
    if (pNoteblock == NULL) { return NULL; }
    pNoteblock->pNext = NULL;
    char* pText = get_ptr_to_text (pNoteblock);
    draw_staff (pText, (topIs2Digits ? 4 : 3), 0);

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
    struct noteblock* pNoteblock = malloc (sizeof (struct noteblock));
    if (pNoteblock == NULL) { return NULL; }
    pNoteblock->pNext = NULL;
    char* pText = get_ptr_to_text (pNoteblock);
    draw_staff (pText, NOTEBLOCK_WIDTH, 0);

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
    struct noteblock* pNoteblock = malloc (sizeof (struct noteblock));
    if (pNoteblock == NULL) { return NULL; }
    pNoteblock->pNext = NULL;
    char* pText = get_ptr_to_text (pNoteblock);
    int width = ((byte >> 4) >= 6) ? NOTEBLOCK_WIDTH : BARLINE_NOTEBLOCK_WIDTHS[byte >> 4];
    draw_staff (pText, width, 0);

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
    struct noteblock* pNoteblock = malloc (sizeof (struct noteblock));
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



//*******************************************************
// High-level functions dealing with multiple noteblocks
//*******************************************************

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


// Updates the parseInfo variable, which stores three sub-values between calls to parse_byte_group.
//   parseInfo byte 1 - byte group type of current byte group.
//   parseInfo byte 2 - byte2 of most recent note-type noteblock, zeroed between measures.
//   parseInfo byte 3 - byte2 of most recent note-type noteblock, preserved across measures.
unsigned int update_parse_info (
    unsigned int  oldParseInfo,     // parseInfo from previous call to parse_byte_group.
    unsigned char newByteGroupType, // BYTE_GROUP_TYPE constant from current call to parse_byte_group.
    unsigned char newByte2          // Used only if new byte group type is note - bits 17-32 of note encoding.
){
    switch (newByteGroupType) {
        case BYTE_GROUP_TYPE_NOTE:
            // Set bytes 2 and 3 to newByte2, byte 1 to newByteGroupType
            return (newByte2 << 16) | (newByte2 << 8) | newByteGroupType;
        case BYTE_GROUP_TYPE_BARLINE:
            // Preserve byte 3, zero byte 2, set byte 1 to newByteGroupType
            return (oldParseInfo & 0x00FF0000) | newByteGroupType;
        default:
            // Preserve bytes 3 and 2, set byte 1 to newByteGroupType
            return (oldParseInfo & 0x00FFFF00) | newByteGroupType;
    }
}


// PARSE_RESULT constants representing the result of trying to parse one or more encoded bytes.

#define PARSE_RESULT_PARSED_NOTEBLOCK      (0) // Parsed one noteblock.
#define PARSE_RESULT_PARSED_ALL            (1) // Parsed all noteblocks in array of bytes.
#define PARSE_RESULT_UNEXPECTED_TERMINATOR (2) // Failed to parse - found terminator byte (0) at invalid location.
#define PARSE_RESULT_INVALID_BYTE          (3) // Failed to parse - found an invalid byte.
#define PARSE_RESULT_INTERNAL_ERROR        (4) // Failed to parse - internal error, such as out of memory.


// Parse one byte group. This usually creates a new noteblock.
int parse_byte_group (
    const unsigned char* pBytes,      // Pointer to array of bytes (0-terminated) from which to read.
    int*                 pIndex,      // Pointer to index in array of bytes. Calling this function usually increases it.
    struct noteblock**   ppNoteblock, // Pointer to pointer to current noteblock (or pointer to NULL if none). If this
        // function creates a new noteblock, *ppNoteblock will point to it afterwards. If a terminator is parsed,
        // *ppNoteblock will be set to NULL.
    unsigned int*        pParseInfo   // Pointer to info stored between calls to this function - see update_parse_info.
    // Returns one of the PARSE_RESULTs.
){
    if (ppNoteblock == NULL) { return PARSE_RESULT_INTERNAL_ERROR; } // *ppNoteblock can be NULL, but ppNoteblock can't

    unsigned char byte1 = pBytes[*pIndex]; ++(*pIndex);
    unsigned char byte2 = 0, byte3 = 0, byte4 = 0; // May be set later depending on byte group type
    struct noteblock* pNewNoteblock = NULL;
    int byteGroupType = byte_group_type (byte1);

    switch (byteGroupType) {
        case BYTE_GROUP_TYPE_INVALID:
            // 1 byte
            return PARSE_RESULT_INVALID_BYTE;
        case BYTE_GROUP_TYPE_TERMINATOR:
            // 1 byte
            *ppNoteblock = NULL;
            return PARSE_RESULT_PARSED_ALL;
        case BYTE_GROUP_TYPE_CLEF:
            // 1 byte
            pNewNoteblock = make_clef (byte1);
            break;
        case BYTE_GROUP_TYPE_KEY_CHANGE:
            // 2 bytes
            byte2 = pBytes[*pIndex]; ++(*pIndex);
            if (byte2 == 0) { return PARSE_RESULT_UNEXPECTED_TERMINATOR; }
            byte3 = pBytes[*pIndex]; ++(*pIndex);
            if (byte3 == 0) { return PARSE_RESULT_UNEXPECTED_TERMINATOR; }
            byte4 = pBytes[*pIndex]; ++(*pIndex);
            if (byte4 == 0) { return PARSE_RESULT_UNEXPECTED_TERMINATOR; }
            unsigned short bits01to16 = ((unsigned short)byte2 << 8) + byte1;
            unsigned short bits17to32 = ((unsigned short)byte4 << 8) + byte3;
            pNewNoteblock = make_key_signature (bits01to16, bits17to32);
            break;
        case BYTE_GROUP_TYPE_TIME_CHANGE:
            // 1 byte
            pNewNoteblock = make_time_signature (byte1);
            break;
        case BYTE_GROUP_TYPE_NOTE:
            // 2 bytes
            byte2 = pBytes[*pIndex]; ++(*pIndex);
            if (byte2 == 0) { return PARSE_RESULT_UNEXPECTED_TERMINATOR; }
            pNewNoteblock = make_note (byte1, byte2, *pParseInfo);
            break;
        case BYTE_GROUP_TYPE_BARLINE:
            // 1 byte
            pNewNoteblock = make_barline (byte1);
            break;
        case BYTE_GROUP_TYPE_DYN_TEXT: {
            // 3 bytes
            // This is the only set of bytes that modifies the current noteblock rather than creating a new one.
            // Dynamics text can't be the first byte group or appear twice consecutively.
            int prevByteGroupType = *pParseInfo & 0xFF;
            if (ppNoteblock == NULL || prevByteGroupType == 0 || prevByteGroupType == BYTE_GROUP_TYPE_DYN_TEXT) {
                return PARSE_RESULT_INVALID_BYTE;
            }
            char* pText = get_ptr_to_text (*ppNoteblock);
            byte2 = pBytes[*pIndex]; ++(*pIndex);
            if (byte2 == 0) { return PARSE_RESULT_UNEXPECTED_TERMINATOR; }
            byte3 = pBytes[*pIndex]; ++(*pIndex);
            if (byte3 == 0) { return PARSE_RESULT_UNEXPECTED_TERMINATOR; }
            draw_dynamics_text_row (pText, byte1, byte2, byte3);
            break;
        }
    }

    // Unless type was dynamics text, a new noteblock should have been created.
    if (pNewNoteblock != NULL) {
        if (*ppNoteblock != NULL) { (*ppNoteblock)->pNext = pNewNoteblock; }
        *ppNoteblock = pNewNoteblock;
    }
    else if (byteGroupType != BYTE_GROUP_TYPE_DYN_TEXT) {
        *ppNoteblock = NULL;
        return PARSE_RESULT_INTERNAL_ERROR; // Probably ran out of memory
    }

    // Update parseInfo
    *pParseInfo = update_parse_info (*pParseInfo, (unsigned char)byteGroupType, byte2);

    return PARSE_RESULT_PARSED_NOTEBLOCK;
}


// Parse array of encoded bytes to create list of noteblocks
int parse_bytes_start_to_end (
    const unsigned char* pBytes,         // Pointer to array of bytes (0b11111111-terminated) from which to read.
    struct noteblock**   pp1stNoteblock, // Will be set to pointer to pointer to first noteblock in list.
    int*                 pErrIndex       // If an error occurs, will be set to its index in *pBytes, otherwise to -1.
    // Returns one of the PARSE_RESULTs
){
    int index = 0;
    unsigned int parseInfo = 0;
    *pp1stNoteblock = NULL; // Set to NULL so parse_byte_group knows it's at the first noteblock
    int parseResult = parse_byte_group (pBytes, &index, pp1stNoteblock, &parseInfo);
    struct noteblock* pNoteblock = *pp1stNoteblock;
    while (parseResult == PARSE_RESULT_PARSED_NOTEBLOCK) {
        parseResult = parse_byte_group (pBytes, &index, &pNoteblock, &parseInfo);
    }
    *pErrIndex = (parseResult == PARSE_RESULT_PARSED_ALL) ? -1 : index - 1;
    return parseResult;
}



//*****************************************************************************
// Functions for converting a list of noteblocks to a string
//
// The string contains one or more staves. Each staff contains 16 newline-terminated rows.
// We handle a user-specified max staff/row width to ensure the string displays properly in the terminal.
// If the width is large enough, there may be only one staff.
//*****************************************************************************


// Create the top row of the current staff.
// For performance, this function is separate from append_staff_row_subsequent, which handles
// the other 15 rows of the staff. This function determines how many noteblocks fit in the
// staff based on the max width. It sets *ppStaffHeadNext accordingly,
// and append_staff_row_subsequent uses that information.
// Originally, append_staff_row_initial and append_staff_row_subsequent were not separated into
// two different functions. I noticed that they used a large portion of runtime, so I separated
// them to improve performance. They are similar in structure and function.
void append_staff_row_initial (
    struct noteblock*  pStaffHead,      // Pointer to first noteblock in current staff.
    struct noteblock** ppStaffHeadNext, // *ppStaffHeadNext will be set to pointer to first noteblock in next staff,
                                        // or NULL if currently in last staff.
    int                row,             // Row number (0 to 15) in staff. Should be the top row, ROW_HI_B.
    char*              str,             // Partially populated character array, in which to append.
    unsigned int*      pIdxInStr,       // Pointer to next index in str. Increased when function called.
    int                maxStaffWidth    // Max number of characters in the staff's string representation (not including
                                        // newline).
){
    unsigned int limitIdxInStr = *pIdxInStr + maxStaffWidth;
    unsigned int unsafeIdxInStr = limitIdxInStr - NOTEBLOCK_WIDTH;
    struct noteblock* pCurrentNoteblock = pStaffHead;
    while (pCurrentNoteblock != NULL) {
        char* pRow = get_ptr_to_row_from_noteblock (pCurrentNoteblock, row);
        if (*pIdxInStr >= unsafeIdxInStr) {
            int width = (pRow[0] != '\0') + (pRow[1] != '\0') + (pRow[2] != '\0') + (pRow[3] != '\0') + (pRow[4] != '\0');
            if (*pIdxInStr + width >= limitIdxInStr) { break; }
        }
        if (pRow[0] != '\0') { str[*pIdxInStr] = pRow[0]; ++(*pIdxInStr); }
        if (pRow[1] != '\0') { str[*pIdxInStr] = pRow[1]; ++(*pIdxInStr); }
        if (pRow[2] != '\0') { str[*pIdxInStr] = pRow[2]; ++(*pIdxInStr); }
        if (pRow[3] != '\0') { str[*pIdxInStr] = pRow[3]; ++(*pIdxInStr); }
        if (pRow[4] != '\0') { str[*pIdxInStr] = pRow[4]; ++(*pIdxInStr); }
        pCurrentNoteblock = pCurrentNoteblock->pNext;
    }
    str[*pIdxInStr] = '\n'; ++(*pIdxInStr);
    *ppStaffHeadNext = pCurrentNoteblock;
}


// Create another row of the current staff.
// Assumes that append_staff_row_initial has already determined which noteblocks are in range
// for the current staff and that pStaffHead and pStaffHeadNext are set accordingly.
void append_staff_row_subsequent (
    struct noteblock* pStaffHead,     // Pointer to first noteblock in current staff.
    struct noteblock* pStaffHeadNext, // Pointer to first noteblock in next staff, or NULL if currently in last staff.
                                      // Call append_staff_row_initial to find this.
    int               row,            // Row number (0 to 15) in staff.
    char*             str,            // Partially populated character array, in which to append.
    unsigned int*     pIdxInStr       // Pointer to next index in str. Increased when function called.
){
    struct noteblock* pCurrentNoteblock = pStaffHead;
    while (pCurrentNoteblock != pStaffHeadNext  && pCurrentNoteblock != NULL) {
        char* pRow = get_ptr_to_row_from_noteblock (pCurrentNoteblock, row);
        if (pRow[0] != '\0') { str[*pIdxInStr] = pRow[0]; ++(*pIdxInStr); }
        if (pRow[1] != '\0') { str[*pIdxInStr] = pRow[1]; ++(*pIdxInStr); }
        if (pRow[2] != '\0') { str[*pIdxInStr] = pRow[2]; ++(*pIdxInStr); }
        if (pRow[3] != '\0') { str[*pIdxInStr] = pRow[3]; ++(*pIdxInStr); }
        if (pRow[4] != '\0') { str[*pIdxInStr] = pRow[4]; ++(*pIdxInStr); }
        pCurrentNoteblock = pCurrentNoteblock->pNext;
    }
    str[*pIdxInStr] = '\n'; ++(*pIdxInStr);
}


// Convert noteblocks to a single string.
char* noteblocks_to_string (
    struct noteblock* p1stNoteblock, // Initial noteblock.
    int               maxStaffWidth  // Max width of a staff in characters. Should be no less than NOTEBLOCK_WIDTH.
    // Returns the result of converting these noteblocks to a single string.
){
    if (p1stNoteblock == NULL || maxStaffWidth < NOTEBLOCK_WIDTH) { return NULL; }

    // Allocate a string with max length we might need if every noteblock fills all 5 columns (no '\0' column)
    unsigned int countNoteblocks = count_noteblocks (p1stNoteblock);
    unsigned int noteblocksPerStaff = maxStaffWidth / NOTEBLOCK_WIDTH; // Assuming all 5 columns used always
    unsigned int countStaves = (countNoteblocks / noteblocksPerStaff) + (countNoteblocks % noteblocksPerStaff > 0);
    unsigned int countChars = (NOTEBLOCK_HEIGHT * NOTEBLOCK_WIDTH * countNoteblocks) // Actual noteblock text
        + ((NOTEBLOCK_HEIGHT + 1) * countStaves) // '\n' at end of each row, including extra seperator row between staves
        + 1; // '\0' at end of string
    char* str = malloc (countChars);
    if (str == NULL) { return NULL; }

    // Loop over staves until last noteblock processed
    unsigned int idxInStr = 0;
    struct noteblock* pStaffHead = p1stNoteblock; // First noteblock in current staff
    while (pStaffHead != NULL) {
        // Loop over rows in staff. Rows are numbered from bottom, but we're printing from top, so loop backwards.
        int row = NOTEBLOCK_HEIGHT - 1;
        struct noteblock* pStaffHeadNext; // Will be set by following function
        append_staff_row_initial (pStaffHead, &pStaffHeadNext, row, str, &idxInStr, maxStaffWidth);
        for (--row; row >= 0; --row) {
            append_staff_row_subsequent (pStaffHead, pStaffHeadNext, row, str, &idxInStr);
        }
        str[idxInStr] = '\n'; ++idxInStr; // Separate staves
        pStaffHead = pStaffHeadNext;
    }
    str[idxInStr] = '\0'; ++idxInStr;
    if (idxInStr > countChars) { free (str); return NULL; } // Sanity check
    return str;
}



//*********
// Main/IO
//*********


// Format a byte in format 0bXXXXXXXX
void format_byte_0b (
    char          byteStr[11], // Output param, char[11] that will be set with format 0bXXXXXXXX\0.
    unsigned char byte         // Byte to format.
){
    strcpy_s (byteStr, 11, "0b00000000"); // strcpy_s also copies the '\0' to byteStr[10]
    for (int i = 0; i < 8; ++i) {
        if ((byte >> i) & 0b1) {
            byteStr[9 - i] = '1';
        }
    }
}


// Format a byte from an array in format 0bXXXXXXXX. If index is out of bounds, use byte 0.
void format_byte_from_index (
    char                 byteStr[11], // Output param, char[11] that will be set with format 0bXXXXXXXX\0.
    const unsigned char* pBytes,      // Pointer to array of bytes (0-terminated) from which to read.
    int                  index        // Index in array of bytes.

){
    size_t bytesLen = strlen (pBytes);
    unsigned char byte = (index < 0 || INT_MAX < bytesLen || (int)bytesLen <= index) ? 0 : pBytes[index];
    format_byte_0b (byteStr, byte);
}


// Size in bytes of largest file we would try to read from.
#define FILE_SIZE_MAX (99999)

// Attempts to open file, decode it, and print music.
void try_read_file (
    char* filepath, // User-entered file path and name.
    char* widthStr  // User-entered string for maximum staff width, or NULL if not entered.
){
    // Find staff width, parsing widthStr if specified
    int widthInt;
    if (widthStr != NULL) {
        widthInt = atoi (widthStr); // Returns 0 if not parsable
        if (widthInt == 0) {
            printf ("  Invalid width\n");
            return;
        }
        else if (widthInt < NOTEBLOCK_WIDTH) {
            printf ("  Invalid width %s < %d\n", widthStr, NOTEBLOCK_WIDTH);
            return;
        }
    }
    else {
        widthInt = INT_MAX; // Will effectively be ignored
    }

    // Open file
    FILE* file;
    errno_t fopenErr = fopen_s ( // Microsoft's enhanced security version of fopen
        &file, filepath, "rb"); // rb: binary read mode
    if (fopenErr || file == NULL) {
        printf ("  Unable to open file %s\n", filepath);
        return;
    }

    // Get file size
    fseek (file, 0, SEEK_END);
    int fileSize = ftell (file);
    rewind (file);

    // Validate file size
    if (fileSize == 0) {
        printf ("  File is empty: %s\n", filepath);
        fclose (file);
        return;
    }
    if (fileSize > FILE_SIZE_MAX) {
        printf ("  File is too long (>%d bytes): %s\n", FILE_SIZE_MAX, filepath);
        fclose (file);
        return;
    }

    // Read file into array, and close file
    unsigned char* pBytes = malloc (fileSize);
    if (pBytes == NULL) {
        printf ("  Memory allocation error\n");
        fclose (file);
        return;
    }
    fread (pBytes, 1, fileSize, file);
    fclose (file);

    // Array of bytes to list of noteblocks
    struct noteblock* p1stNoteblock;
    int errIndex;
    int parseResult;
    parseResult = parse_bytes_start_to_end (pBytes, &p1stNoteblock, &errIndex);
    if (parseResult != PARSE_RESULT_PARSED_ALL) {
        switch (parseResult) {
            case PARSE_RESULT_INVALID_BYTE: {
                char byteStr[11];
                format_byte_from_index (byteStr, pBytes, errIndex);
                printf ("  Invalid byte %s at location #%d\n", byteStr, errIndex);
                break;
            }
            case PARSE_RESULT_UNEXPECTED_TERMINATOR:
                printf ("  Invalid terminator byte 0b00000000 at location #%d\n", errIndex);
                break;
            default:
                printf ("  Internal error while parsing noteblocks\n");
        }
        free_noteblocks (p1stNoteblock); free (pBytes);
        return;
    }

    // List of noteblocks to string
    char* str = noteblocks_to_string (p1stNoteblock, widthInt);
    if (str == NULL) {
        printf ("  Internal error while converting noteblocks to string\n");
        free_noteblocks (p1stNoteblock); free (pBytes);
        return;
    }
    printf ("%s", str);
    free (str); free_noteblocks (p1stNoteblock); free (pBytes);
}


// Returns example string printed when user uses cmd line option -v.
// If an error occurs, prints error information and returns NULL.
char* str_example ()
{
    int parseResult;
    struct noteblock* p1stNoteblock;
    int errIndex;
    parseResult = parse_bytes_start_to_end (EXAMPLE_BYTES, &p1stNoteblock, &errIndex);
    if (parseResult != PARSE_RESULT_PARSED_ALL) {
        char byteStr[11];
        format_byte_from_index (byteStr, EXAMPLE_BYTES, errIndex);
        char* noteblockCountStr = (p1stNoteblock == NULL) ? "no" : "at least one";
        printf ("  Internal error: parse result %d; error index %d; byte %s; %s noteblock exists\n",
            parseResult, errIndex, byteStr, noteblockCountStr);
        free_noteblocks (p1stNoteblock);
        return NULL;
    }

    char* str = noteblocks_to_string (p1stNoteblock, EXAMPLE_WIDTH);
    free_noteblocks (p1stNoteblock);
    return str;
}


// Test performance by constructing str_example count times
void test_performance (
    char* countStr // String representing how many times to call str_example. Should be >= 10.
){
    int countInt = atoi (countStr); // Returns 0 if not parsable
    if (countInt < 10) {
        if (countInt == 0) {
            printf ("  Invalid count\n");
        }
        else {
            printf ("  Invalid count: %s < 10\n", countStr);
        }
        return;
    }

    // The following is over-optimized for the speed of the loop.
    // In particular, the loop does direct comparison to 0 with no modulus involved.
    int tenthOfCount = countInt / 10;
    int tenthsDone = 0; // How many times we have looped count/10 times
    int i = tenthOfCount + (countInt % 10); // i will count down to 0 ten times
    printf ("  Done: 00%%");
    time_t time0, time1;
    time (&time0);
    while (1) {
        // Meat of loop
        char* s = str_example ();
        free (s);
        // Rest of loop
        --i;
        if (i) {
            continue;
        }
        else {
            ++tenthsDone;
            printf (" %d0%%", tenthsDone);
            if (tenthsDone < 10) {
                i = tenthOfCount; // Reset for next countdown
            }
            else {
                break;
            }
        }
    }
    time (&time1);
    printf ("\n  Example output constructed %s times in <%d seconds\n", countStr, ((int)time1 - (int)time0 + 1));
}


// Main entry point that a user can call from the command line
int main (
    int   argc,  // Count of command-line argument strings, including program name
    char* argv[] // Array of command-line argument strings (first actual argument is argv[1])
){
    if (argc == 2 && strcmp (argv[1], "-e") == 0) {
        printf (STR_ENCODING);
    }
    else if (argc == 2 && strcmp (argv[1], "-v") == 0) {
        char* strExample = str_example ();
        if (strExample != NULL) {
            printf (strExample);
            free (strExample);
        }
    }
    else if (argc == 2 && strcmp (argv[1], "-p") == 0) {
        printf ("  Count argument required for option -p\n");
    }
    else if (argc == 2 && strcmp (argv[1], "-h") == 0) {
        printf (STR_HELP);
    }
    else if (argc == 2) {
        try_read_file (argv[1], NULL);
    }
    else if (argc == 3 && strcmp (argv[1], "-p") == 0) {
        test_performance (argv[2]);
    }
    else if (argc == 3) {
        try_read_file (argv[1], argv[2]);
    }
    else {
        printf (STR_HELP);
    }
    return 0;
}

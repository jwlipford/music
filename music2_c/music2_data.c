//*****************************************************************************************************
// music2_data.c
// This file contains data used by the -v options, simulating data the program could read from a file.
//*****************************************************************************************************


//*********************
// General example song
//*********************

// This width splits the string representation of EXAMPLE_BYTES neatly into two staves.
const int EXAMPLE_WIDTH = 85;

// General example song to illustrate visual style of output.
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
    0b10000100, // Left repeat barline :| slim
    0b00100101, 0b00100100, // High C, half, tied to nothing
    0           // Terminator
};



//***********************************************
// Detailed examples of specific byte group types
//***********************************************

// Standard width to use for detailed examples.
const int DTL_WIDTH = 80;

// Detailed example of clefs. Displays all three clefs.
const unsigned char DTL_BYTES_CLEF[] = {
    0b00100000, // Treble clef
    0b01010100, // Blank column
    0b01100000, // Bass clef
    0b01010100, // Blank column
    0b10100000, // Percussion clef
    0           // Terminator
};

// Detailed example of key changes. Has every standard key signature, plus a few others.
const unsigned char DTL_BYTES_KEY_CHANGE[] = {
    0b00000011, 0b11000001, 0b00000111, 0b11000000, // F  major key
    0b00000100, // Single barline
    0b00000011, 0b11001001, 0b00000111, 0b11000000, // Bb major key
    0b00000100, // Single barline
    0b10000011, 0b11001001, 0b00000111, 0b11000000, // Eb major key
    0b00000100, // Single barline
    0b10000011, 0b11001101, 0b00000111, 0b11000000, // Ab major key
    0b00000100, // Single barline
    0b11000011, 0b11001101, 0b00000111, 0b11000000, // Db major key
    0b00000100, // Single barline
    0b11000011, 0b11001111, 0b00000111, 0b11000000, // Gb major key
    0b00000100, // Single barline
    0b11100011, 0b11001111, 0b00000111, 0b11000000, // Cb major key
    0b00000100, // Single barline
    0b11100011, 0b11001111, 0b11100111, 0b11000110, // Bb major key with naturals compared to Cb major key
    0b00000100, // Single barline
    0b00000011, 0b11001001, 0b00000111, 0b11001001, // C  major key with naturals compared to Bb major key
    0b00000100, // Single barline
    0b00000011, 0b11000000, 0b00000111, 0b11000000, // C  major key, flats direction
    // Staff break
    0b00000111, 0b11000000, 0b00000111, 0b11010000, // G  major key
    0b00000100, // Single barline
    0b00000111, 0b11000000, 0b00000111, 0b11010010, // D  major key
    0b00000100, // Single barline
    0b00000111, 0b11000000, 0b00000111, 0b11110010, // A  major key
    0b00000100, // Single barline
    0b00000111, 0b11000000, 0b00000111, 0b11110110, // A  major key
    0b00000100, // Single barline
    0b00000111, 0b11000000, 0b10000111, 0b11110110, // B  major key
    0b00000100, // Single barline
    0b00000111, 0b11000000, 0b10000111, 0b11111110, // F# major key
    0b00000100, // Single barline
    0b00000111, 0b11000000, 0b10000111, 0b11111111, // C# major key
    0b00000100, // Single barline
    0b10000111, 0b11101101, 0b10000111, 0b11111111, // D  major key with naturals compared to C# major key
    0b00000100, // Single barline
    0b00000111, 0b11010010, 0b00000111, 0b11010010, // C  major key with naturals compared to D major key
    0b00000100, // Single barline
    0b00000111, 0b11000000, 0b00000111, 0b11000000, // C  major key sharps direction
    0b00000100, // Single barline
    0b10000011, 0b00000001, 0b10000010, 0b11000010, // A~, Bb, C#, flats direction
    0b00000100, // Single barline
    0b10000111, 0b00000001, 0b10000010, 0b11000010, // A~, Bb, C#, sharps direction
    0b00000100, // Single barline
    0b11111011, 0b11111111, 0b00000111, 0b11000000, // All flats, normal flats direction
    0b00000100, // Single barline
    0b00000111, 0b11000000, 0b11111111, 0b11111111, // All sharps, normal sharps direction
    0b00000100, // Single barline
    0b11111011, 0b11111111, 0b11111111, 0b11111111, // All naturals, flats direction
    0           // Terminator
};

// Detailed example of time changes.
// Displays all 16 top numbers and all 4 bottom numbers, but not all combinations thereof.
const unsigned char DTL_BYTES_TIME_CHANGE[] = {
    0b00000010, //  1|1
    0b00010110, //  2|2
    0b00101010, //  3|4
    0b00111110, //  4|8
    0b01000010, //  5|1
    0b01010110, //  6|2
    0b01101010, //  7|4
    0b01111110, //  8|8
    0b10000010, //  9|1
    0b10010110, // 10|2
    0b10101010, // 11|4
    0b10111110, // 12|8
    0b11000010, // 13|1
    0b11010110, // 14|2
    0b11101010, // 15|4
    0b11111110, // 16|8
    0           // Terminator
};

// Detailed example of notes and rests. Just one for now.
const unsigned char DTL_BYTES_NOTE[] = {
    0b00010001, 0b0000011, // One note
    0           // Terminator
};

// Detailed example of dynamics text. Displays all valid characters.
const unsigned char DTL_BYTES_TEXT[] = {
    0b01000100, // Both repeats barline (width 4)
    0b01101000, 0b10000111, 0b00011001, // Dynamics text "cdef\0"
    0b01000100, // Both repeats barline
    0b10101000, 0b11001011, 0b00011101, // Dynamics text "mprs\0"
    0b01000100, // Both repeats barline
    0b00101000, 0b01000011, 0b00010101, // Dynamics text " <>.\0"
    0           // Terminator
};

// Detailed example of barlines. Displays all of them.
const unsigned char DTL_BYTES_BARLINE[] = {
    0b00100000, // Treble clef
    0b00000100, // Single wide
    0b00100000, // Treble clef
    0b01100100, // Single slim
    0b00100000, // Treble clef
    0b00010100, // Double wide
    0b00100000, // Treble clef
    0b01110100, // Double slim
    0b00100000, // Treble clef
    0b00100100, // Left repeat wide
    0b00100000, // Treble clef
    0b10000100, // Left repeat slim
    0b00100000, // Treble clef
    0b00110100, // Right repeat wide
    0b00100000, // Treble clef
    0b10010100, // Right repeat slim
    0b00100000, // Treble clef
    0b01000100, // Both repeats slim
    0b00100000, // Treble clef
    0b01010100, // Blank column
    0b00100000, // Treble clef
    0           // Terminator
};

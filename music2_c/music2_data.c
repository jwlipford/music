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
    0b00000001, 0b01100000, // Eighth rest
    0b00000101, 0b00110110, 0b11000001, // Low G, eighth note, stem length 4, 1 right beam
    0b00101000, 0b10111010, 0b00100010, // Dynamics text " mp  "
    0b00000101, 0b10100111, 0b11000101, // Low A, eighth note, stem length 3, 1 left and right beam, dotted
    0b00000101, 0b10110110, 0b11000101, // Low G, eighth note, stem length 4, 1 left and right beam, dotted
    0b00000101, 0b00100111, 0b11000100, // Low A, eighth note, stem length 3, 1 left beam
    0b00000001, 0b01010100, // Low E, quarter
    0b00000100, // Single barline
    0b00000001, 0b01100000, // Eighth rest
    0b00000101, 0b00110110, 0b11000001, // Low G, eighth note, stem length 4, 1 right beam
    0b00000101, 0b10100111, 0b11000101, // Low A, eighth note, stem length 3, 1 left and right beam, dotted
    0b00000101, 0b10110110, 0b11000101, // Low G, eighth note, stem length 4, 1 left and right beam, dotted
    0b00000101, 0b00100111, 0b11000100, // Low A, eighth note, stem length 3, 1 left beam
    0b00000001, 0b01010100, // Low E, quarter
    0b00000100, // Single barline
    0b00000001, 0b01100000, // Eighth rest
    0b00000101, 0b00110110, 0b11000001, // Low G, eighth note, stem length 4, 1 right beam
    0b00000101, 0b10100111, 0b11000101, // Low A, eighth note, stem length 3, 1 left and right beam, dotted
    0b00000101, 0b10110110, 0b11000101, // Low G, eighth note, stem length 4, 1 left and right beam, dotted
    0b00000101, 0b00100111, 0b11000100, // Low A, eighth note, stem length 3, 1 left beam
    0b00000001, 0b01010100, // Low E, quarter
    0b00000100, // Single barline
    0b00000001, 0b01100000, // Eighth rest
    0b00000101, 0b00110110, 0b11000001, // Low G, eighth note, stem length 4, 1 right beam
    0b00101000, 0b00110010, 0b00110011, // Dynamics text "  <<<"
    0b00000101, 0b10100111, 0b11000101, // Low A, eighth note, stem length 3, 1 left and right beam, dotted
    0b00111000, 0b00110011, 0b00110011, // Dynamics text "<<<<<"
    0b00000101, 0b10110110, 0b11000101, // Low G, eighth note, stem length 4, 1 left and right beam, dotted
    0b00111000, 0b00110011, 0b00110011, // Dynamics text "<<<<<"
    0b00000101, 0b00100111, 0b11000100, // Low A, eighth note, stem length 3, 1 left beam
    0b00111000, 0b00110011, 0b00110011, // Dynamics text "<<<<<"
    0b00000001, 0b01100100, // Low E, eighth
    0b00111000, 0b00110011, 0b00110011, // Dynamics text "<<<<<"
    0b00100101, 0b01111001, 0b11000010,  // High C, sixteenth note, downward, stem length 4, 2 right beams, staccato
    0b00111000, 0b10010010, 0b00010010, // Dynamics text "< f  "
    0b10100101, 0b01111001, 0b11001000,  // High C, sixteenth note, downward, stem length 4, 2 left beams, staccato, tied
    0b10000100, // Left repeat barline :| slim
    0b10000001, 0b01001001, // High C, half, tied to nothing
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

// Detailed example of rests.
const unsigned char DTL_BYTES_REST[] = {
    0b00000001, 0b00100000, // Breve rest
    0b00000001, 0b00110000, // Whole rest
    0b00000001, 0b01000000, // Half rest
    0b00000001, 0b01010000, // Quarter rest
    0b00000001, 0b01100000, // Eighth rest
    0b00000001, 0b01110000, // Sixteenth rest
    0b00000001, 0b10100000, // Breve rest, dotted
    0b00000001, 0b10110000, // Whole rest, dotted
    0b00000001, 0b11000000, // Half rest, dotted
    0b00000001, 0b11010000, // Quarter rest, dotted
    0b00000001, 0b11100000, // Eighth rest, dotted
    0b00000001, 0b11110000, // Sixteenth rest, dotted
    0           // Terminator
};

// Detailed example of non-beamed notes.
const unsigned char DTL_BYTES_NOTE[] = {
    // Pitches
    0b00111010, // 4|4 time change
    0b00000001, 0b01110001, // Low B, sixteenth
    0b00000001, 0b01110010, // Low C, sixteenth
    0b00000001, 0b01110011, // Low D, sixteenth
    0b00000001, 0b01110100, // Low E, sixteenth
    0b00000001, 0b01110101, // Low F, sixteenth
    0b00000001, 0b01110110, // Low G, sixteenth
    0b00000001, 0b01110111, // Low A, sixteenth
    0b00000001, 0b01111000, // Mid B, sixteenth
    0b00000001, 0b01111001, // High C, sixteenth
    0b00000001, 0b01111010, // High D, sixteenth
    0b00000001, 0b01111011, // High E, sixteenth
    0b00000001, 0b01111100, // High F, sixteenth
    0b00000001, 0b01111101, // High G, sixteenth
    0b00000001, 0b01111110, // High A, sixteenth
    0b00000001, 0b01101111, // High B, eighth
    0b00000100, // Single barline
    // Accidentals and Articulations
    0b00000001, 0b01010111, // Low A, quarter
    0b00101001, 0b01011000, // Mid B, quarter, flat, staccato
    0b01010001, 0b01011001, // High C, quarter, natural, accent
    0b01111001, 0b01011010, // High D, quarter, sharp, tenuto
    0b00000100, // Single barline
    0b01010100, // Blank column
    // Durations
    0b10010110, // 10|2 time change
    0b00000001, 0b10101001, // High C, breve, dotted
    0b00000001, 0b00101001, // High C, breve, not dotted
    0b00000100, // Single barline
    0b10011010, // 10|4 time change
    0b00000001, 0b10111001, // High C, whole, dotted
    0b00000001, 0b00111001, // High C, whole, not dotted
    0b00000100, // Single barline
    0b01001010, // 5|4 time change
    0b00000001, 0b11001001, // High C, half, dotted
    0b00000001, 0b01001001, // High C, half, not dotted
    0b00000100, // Single barline
    0b01001110, // 5|8 time change
    0b00000001, 0b11011001, // High C, quarter, dotted
    0b00000001, 0b01011001, // High C, quarter, not dotted
    0b00000100, // Single barline
    0b01001110, // 5|8 time change
    0b00000001, 0b11101001, // High C, eighth, dotted
    0b00000001, 0b01101001, // High C, eighth, not dotted
    0b00000001, 0b11111001, // High C, sixteenth, dotted
    0b00000001, 0b01111001, // High C, sixteenth, not dotted
    0b00000001, 0b01110000, // Sixteenth rest
    0b00000001, 0b11111001, // High C, sixteenth, dotted
    0b00000100, // Single barline
    // Ties
    0b00011010, // 2|4 time change
    0b10000001, 0b01010111, // Low A, quarter, not dotted, tied
    0b00000001, 0b01100111, // Low A, eighth, not dotted
    0b10000001, 0b01101001, // High C, eighth, not dotted, tied across barline
    0b01100100, // Single barline, slim
    0b00000001, 0b01101001, // High C, eighth, not dotted
    0b10000001, 0b01100111, // Low A, eighth, not dotted, tied
    0b00000001, 0b01010111, // Low A, quarter, not dotted
    0b00000100, // Single barline
    0b10000001, 0b01111001, // High C, sixteenth, not dotted, tied
    0b10010001, 0b11101001, // High C, eighth, natural, dotted, tied
    0b00000001, 0b01011001, // High C, quarter, not dotted
    0b00010100, // Double wide barline
    0           // Terminator
};

// Detailed example of beamed notes.
const unsigned char DTL_BYTES_BEAMED_NOTE[] = {
    0b00111010, // 4|4 time change
    // Four beamed upward sixteenth notes, low B to low E, stem lengths 4 to 1
    0b00000101, 0b00110001, 0b11000010,
    0b00000101, 0b00100010, 0b11001010,
    0b00000101, 0b00010011, 0b11001010,
    0b00000101, 0b00000100, 0b11001000,
    // Four beamed upward sixteenth notes, low F to mid B, stem lengths 4 to 1
    0b00000101, 0b00110101, 0b11000010,
    0b00000101, 0b00100110, 0b11001010,
    0b00000101, 0b00010111, 0b11001010,
    0b00000101, 0b00001000, 0b11001000,
    // Two beamed downward sixteenth notes, high C to high D, stem lengths 3 to 4
    0b00000101, 0b01101001, 0b11000010,
    0b00000101, 0b01111010, 0b11001000,
    // Two beamed downward sixteenth notes, high E to high F, stem lengths 3 to 4
    0b00000101, 0b01101011, 0b11000010,
    0b00000101, 0b01111100, 0b11001000,
    // Two beamed sixteenth and one beamed eighth note, high G to high B, stem lengths 2 to 4
    0b00000101, 0b01011101, 0b11000010,
    0b00000101, 0b01101110, 0b11011010,
    0b00000101, 0b01111111, 0b11000100,
    0b00000100, // Single barline
    // Four quarter notes, low A to high D, stem length 2
    0b00000101, 0b00010111, 0b11000000, // None, none, upward
    0b00101101, 0b00011000, 0b11000000, // Flat, staccato, upward
    0b01010101, 0b01011001, 0b11000000, // Natural, accent, downward
    0b01111101, 0b01011010, 0b11000000, // Sharp, tenuto, downward
    0b00000100, // Single barline
    // Two quarter notes on high C, downward, stem length 3
    0b01001110, // 5|8 time change
    0b00000101, 0b11101001, 0b11000000, // Dotted quarter note
    0b00000101, 0b01101001, 0b11000000, // Quarter note
    0b00000100, // Single barline
    // Several pairs of beamed notes on high C, downward, and one rest
    0b00101010, // 3|4 time change
    0b00000101, 0b11101001, 0b11000001, // Dotted eighth note, stem length 3, 1 right beam
    0b00000101, 0b01101001, 0b11000100, // Eighth note, stem length 3, 1 left beam
    0b00000101, 0b11111001, 0b11000010, // Dotted sixteenth note, stem length 4, 2 right beams
    0b00000101, 0b01111001, 0b11001000, // Sixteenth note, stem length 4, 2 left beams
    0b00000101, 0b11111001, 0b11000011, // Dotted 32nd note, stem length 4, 3 right beams
    0b00000101, 0b01111001, 0b11001100, // 32nd note, stem length 4, 3 left beams
    0b00000001, 0b01110000, // Sixteenth rest
    0b00000101, 0b11111001, 0b11010011, // Dotted 32nd note, stem length 4, 3 right beams
    0b00000101, 0b11111001, 0b11001000, // Dotted sixteenth note, stem length 4, 2 left beams
    0b00000100, // Single barline
    // Notes with ties
    0b00011010, // 2|4 time change
    0b10000101, 0b00100111, 0b11000000, // Low A, upward, stem length 3, no beams, tied
    0b00000101, 0b01010111, 0b11000001, // Low A, downward, stem length 2, 1 right beam
    0b10000101, 0b01111001, 0b11000100, // High C, downward, stem length 4, 1 left beam, tied across barline
    0b01100100, // Single barline, slim
    0b00000101, 0b01111001, 0b11000001, // High C, downward, stem length 4, 1 right beam
    0b10000101, 0b01010111, 0b11000100, // Low A, downward, stem length 2, 1 left beam, tied
    0b00000101, 0b00100111, 0b11000000, // Low A, upward, stem length 3, no beams
    0b00000100, // Single barline
    0b10000101, 0b01101001, 0b11010010, // High C, sixteenth note, downward, stem length 3, 2 right beams, tied
    0b10010101, 0b11101001, 0b11000100, // High C, dotted eighth note, downward, stem length 3, 1 left beam, natural, tied
    0b00000101, 0b01101001, 0b11000000, // High C, quarter note, downward, stem length 3
    0b00000100, // Single barline
    // Eight middle B quarter notes with different orientations and stem lengths
    0b01111010, // 8|4 time change
    0b00000101, 0b01111000, 0b11000000, // Downward, stem length 4
    0b00000101, 0b01101000, 0b11000000, // Downward, stem length 3
    0b00000101, 0b01011000, 0b11000000, // Downward, stem length 2
    0b00000101, 0b01001000, 0b11000000, // Downward, stem length 1
    0b00000101, 0b00001000, 0b11000000, // Upward, stem length 1
    0b00000101, 0b00011000, 0b11000000, // Upward, stem length 2
    0b00000101, 0b00101000, 0b11000000, // Upward, stem length 3
    0b00000101, 0b00111000, 0b11000000, // Upward, stem length 4
    0b00010100, // Double wide barline
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

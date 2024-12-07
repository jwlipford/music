//******************************************************************************************
// music2_data.h
// Contains data used by the -v options, simulating data the program could read from a file.
//******************************************************************************************

#pragma once

// This width splits the string representation of EXAMPLE_BYTES neatly into two staves.
const int EXAMPLE_WIDTH;

// General example song to illustrate visual style of output.
const unsigned char EXAMPLE_BYTES[];

// Standard width to use for detailed examples.
const int DTL_WIDTH;

// Detailed example of clefs. Displays all three clefs.
const unsigned char DTL_BYTES_CLEF[];

// Detailed example of key changes. Has every standard key signature, plus a few others.
const unsigned char DTL_BYTES_KEY_CHANGE[];

// Detailed example of time changes.
// Displays all 16 top numbers and all 4 bottom numbers, but not all combinations thereof.
const unsigned char DTL_BYTES_TIME_CHANGE[];

// Detailed example of notes and rests. Just one for now.
const unsigned char DTL_BYTES_NOTE[];

// Detailed example of dynamics text. Displays all valid characters.
const unsigned char DTL_BYTES_TEXT[];

// Detailed example of barlines. Displays all six barlines.
const unsigned char DTL_BYTES_BARLINE[];

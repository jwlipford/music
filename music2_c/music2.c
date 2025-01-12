//*****************************************************************************************************
// music2.c
// The program takes a bit string as input, converts to music notation which is printed.
// This is primarily an exercise in squeezing high information density out of a small number of bytes.
// This is version 2.
// This file contains the help text and the main entry point of the program.
//*****************************************************************************************************


// External inclusions
#include <stddef.h> // NULL
#include <stdio.h>  // printf
#include <string.h> // strcmp

// Internal inclusions
#include "music2_general2.h"


//*******************************************************
// Help text - this doubles as an informal specification
//*******************************************************

// Command line arguments
const char* STR_HELP =
"  music.exe prints sheet music (drawn with ASCII characters) from an encoded file. Options:\n"
"    music.exe -e                   Print file encoding information\n"
"    music.exe -v                   Print example of visual style of output\n"
"    music.exe -v <type>            Print examples of a specific byte group type -\n"
"                                   type = clef, key, time, note, text, or barline\n"
"    music.exe <filepath>           Read a file and print music on a continuous staff\n"
"    music.exe <filepath> <width>   Read a file and print music with a maximum page width (min 5, max 255)\n"
"    music.exe -p <count>           Test performance by repeatedly constructing the example from option -v\n"
"    music.exe -p <count> <type>    Test performance by repeatedly constructing the example from option -v <type>\n";

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
"    Bit  5-8: Type of barline - single wide (0), double wide (1), left repeat wide (2),\n"
"              right repeat wide (3), both repeats slim (4), blank column (5), single slim (6),\n"
"              double slim (7), left repeat slim (8), right repeat slim (9)\n"
"  Dynamics text (3 bytes):\n"
"    Applies to the previous noteblock. (Non-note noteblocks might need dynamics text from part of a\n"
"    crescendo or decrescendo under them.) Invalid if this is the first byte or there are two in a row.\n"
"    Bits 1-4: Always 1000\n"
"    Bits 5-8, 9-12, 13-16, 17-20, 21-24:\n"
"      Each group of four bits represents one of these characters):\n"
"        Invalid (0), null (1), space (2), '<' (3), '>' (4), '.' (5), 'c' (6), 'd' (7), 'e' (8),\n"
"        'f' (9), 'm' (10), 'p' (11), 'r' (12), 's' (13), unused (14-15)\n"
"      These 12 characters can make text such as \" ppp \", \"cresc\", \" decr\", \" mp<<\", \"<<f>>\", etc.\n"
"      When the modified noteblock is x < 5 characters wide, use null (1) for characters x+1 to 5 of the\n"
"      dynamics text.\n"
"  Clef (1 byte):\n"
"    Elsewhere in these descriptions, pitch names assume treble clef, but here you can draw a different clef.\n"
"    Bits 1-6: Always 100000\n"
"    Bits 7-8: Type - Treble (0), bass (1), percussion (2)\n";



//******************
// Main entry point
//******************

// Main entry point that a user can call from the command line
int main (
    int   argc,  // Count of command-line argument strings, including program name
    char* argv[] // Array of command-line argument strings (first actual argument is argv[1])
){
    if ((argc == 2 || argc == 3) && strcmp (argv[1], "-v") == 0) {
        char* typeArg = (argc == 2) ? NULL : argv[2];
        show_example (typeArg);
    }
    else if (argc == 2 && strcmp (argv[1], "-e") == 0) {
        printf (STR_ENCODING);
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
    else if ((argc == 3 || argc == 4) && strcmp (argv[1], "-p") == 0) {
        char* typeArg = (argc == 3) ? NULL : argv[3];
        test_performance (argv[2], typeArg);
    }
    else if (argc == 3) {
        try_read_file (argv[1], argv[2]);
    }
    else {
        printf (STR_HELP);
    }
    return 0;
}

// music_cs.cs
// Translated from C (music.c) to C#
// Takes a bit string as input, converts to music notation which is printed.
// Original C program was fundamentally an exercise in squeezing high information density out of a small number of bytes.
// This translated program is an exercise is comparing the two languages' features/syntax/performance.
// It turns out that the C version is a lot faster.


using System;
using System.Diagnostics;
using System.IO;
using System.Text;

namespace MusicCS {
    #region Music class
    
    static partial class Music {
        #region Long strings, data
        
        // Unfortunately, Visual Studio 2019 doesn't support .NET 7, which is needed for C# 11,
        // which has """raw string literals""". So, we have to use ugly string concatenation. :(

        // Command line arguments
        const string STR_HELP =
        "  music.exe prints sheet music (drawn with ASCII characters) from an encoded file. Options:\n" +
        "    music_cs.exe -e                   Print file encoding information\n" +
        "    music_cs.exe -v                   Print example of visual style of output\n" +
        "    music_cs.exe <filepath>           Read a file and print music on a continuous staff\n" +
        "    music_cs.exe <filepath> <width>   Read a file and print music with a maximum page width (min 5, max 255)\n" +
        "    music_cs.exe -p <count>           Test performance by repeatedly constructing the example from option -v\n";
        
        // File encoding
        const string STR_ENCODING =
        "  FILE CREATION\n" +
        "  To create an encoded file, you can use the PowerShell set-content cmdlet. For example:\n" +
        "    $b=[byte[]]@(0x53,0x61,0x75,0x63,0x65)\n" +
        "    set-content encoded_song $b -encoding byte\n" +
        "  FILE ENCODING\n" +
        "  The file consists of groups of bytes that represent \"noteblocks\", which are rectangles of text 16\n" +
        "  characters high and at most 5 characters wide. At most 4 bytes represent one noteblock.\n" +
        "  In the following descriptions, we count bits from the right, the 1's place, starting at 1.\n" +
        "  For example, in the bit string 10010, bits 2 and 5 are the 1s; bits 1, 3, and 4 are the 0s.\n" +
        "  For two bytes, their bits numbered in base 32 are: 87654321 GFEDCBA9.\n" +
        "  Terminator (1 byte):\n" +
        "    Bits 1-8: Always 00000000\n" +
        "  Note (2 bytes):\n" +
        "    Bits 1-2:   Always 01\n" +
        "    Bits 3-6:   Rest (0) or pitches low B (1) to middle B (8) to high B (15)\n" +
        "    Bits 7-8:   Accidentals - none (0), flat ('b') (1), natural ('~') (2), or sharp ('#') (3)\n" +
        "    Bits 9-12:\n" +
        "      Appearance - Invalid (0), unused (1), Breve (2), whole (3), half (4), quarter (5), eighth (6-14 even),\n" +
        "      sixteenth (7-15 odd). For eighths and sixteenths, there are five encodings each that indicate whether\n" +
        "      the note is flagged or beamed and, if beamed, the beam height:\n" +
        "        Flagged (6-7) VS beamed (8-15).\n" +
        "        Beamed on left only with number of left-side beams determined by previous note (12-15), VS\n" +
        "          beamed on right at least once with number of left-side beams determined by previous note (8-11).\n" +
        "        Beam two spaces away (8-9, 12-13) VS three spaces away (10-11, 14-15).\n" +
        "    Bit  13:    Dotted\n" +
        "    Bit  14:    Tie/slur after\n" +
        "    Bits 15-16: Articulations: None (0), staccato (1), accent (2), tenuto (3)\n" +
        "  Time change (1 byte):\n" +
        "    Bits 1-2: Always 10\n" +
        "    Bits 3-4: Bottom number - 1, 2, 4, or 8 (encoded as 0 to 3)\n" +
        "    Bits 5-8: Top number - 1 to 16 (encoded as 0 to 15)\n" +
        "  Key change (4 bytes):\n" +
        "    Bits 1-2: Always 11\n" +
        "    Bit  3:   Arrangement of accidentals - Resembling Db major scale (0) or B major scale (1)\n" +
        "    Bits 4-14, 20-30\n" +  
        "      Pitches go from low D (bits 4 and 20) to high G (bits 14 and 30).\n" +
        "      If a pitch has a 1 in the first bit string but not the second, it is flat ('b').\n" +
        "      If a pitch has a 1 in the second bit string but not the first, it is sharp ('#').\n" +
        "      If a pitch has a 1 in both bit strings, it is natural ('~').\n" +
        "    Bits 15-16, 17-19, 31-32:\n" +
        "      Use these bits to make sure each byte has at least one 1 so it's not a terminator.\n" +
        "  Barline (1 byte):\n" +
        "    Bits 1-4: Always 0100\n" +
        "    Bit  5-7: Type of barline -\n" +
        "              single (0), double (1), left repeat (2), right repeat (3), both repeats (4), blank column (5)\n" +
        "    Bit  8:   Unused\n" +
        "  Dynamics text (3 bytes):\n" +
        "    Applies to the previous noteblock. (Non-note noteblocks might need to have part of a crescendo/\n" +
        "    decrescendo under them.) Invalid if this is the first byte.\n" +
        "    Bits 1-4: Always 1000\n" +
        "    Bits 5-8, 9-12, 13-16, 17-20, 21-24:\n" +
        "      Each group of four bits represents one of these characters (encoded as 1-13, 0 invalid, 14-15 unused):\n" +
        "      Null, space, '<', '>', '.', 'c', 'd', 'e', 'f', 'm', 'p', 'r', 's'\n" +
        "      These 12 characters can make text such as \" ppp \", \"cresc\", \" decr\", \" mp<<\", \"<<f>>\", etc.\n" +
        "  Clef (1 byte):\n" +
        "    Elsewhere in these descriptions, pitch names assume treble clef, but here you can draw a different clef.\n" +
        "    Bits 1-6: Always 100000\n" +
        "    Bits 7-8: Type - Treble (0), bass (1), percussion (2)\n" +
        "  If you ever see a capital 'E' or the string \"ERROR\" in the generated music, your file has invalid input.\n";

        // Example song to illustrate visual style of output. Used by str_example function.
        static readonly byte[] EXAMPLE_BYTES = {
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
            0b00101000, 0b00110010, 0b00110011, // Dynamics text   <<<"
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
        
        #endregion
    }
    
    #endregion
    
    #region Rows enum
    
    // Rows (some unused, but defining all; 0-15; can be looped forwards/backwards)
    enum Row {
        Text, LoB, LoC, LoD, LoE, LoF, LoG, LoA, MdB, HiC, HiD, HiE, HiF, HiG, HiA, HiB,
        Min = Text, Max = HiB
    }
    
    #endregion

    #region Noteblock class
    
    // Structure for text used by a note, time signature, key signature, barline, etc.
    // Also has pointer to next noteblock in linked list of noteblocks

    class Noteblock {
        public const byte WIDTH  =  5;
        public const byte HEIGHT = 16;
        
        public Noteblock? Next = null;
        // Next noteblock
        
        private readonly char[,] text = new char[HEIGHT, WIDTH]; // Readonly pointer, not readonly array contents
        // 2D array of text for the noteblock. Not a pointer.
        // If <5 characters wide, terminate with \0, but make sure each row is same length.
        
        public char GetChar (
            Row row, // Row (0-15)
            int col  // Column (0-4)
        ){
            return this.text[(int)row, col];
        }
        
        public void SetChar (
            Row  row, // Row (0-15)
            int  col, // Column (0-4)
            char val  // Value to set
        ){
            this.text[(int)row, col] = val;
        }
        
        public void DrawRow (
            Row row, // Row (0-15)
            // Below: Characters to copy to the 5-character long row. Pass null to use existing char.
            char? c0, char? c1, char? c2, char? c3, char? c4
        ){
            if (c0 != null) this.text[(int)row, 0] = (char)c0;
            if (c1 != null) this.text[(int)row, 1] = (char)c1;
            if (c2 != null) this.text[(int)row, 2] = (char)c2;
            if (c3 != null) this.text[(int)row, 3] = (char)c3;
            if (c4 != null) this.text[(int)row, 4] = (char)c4;
        }
        
        public void DrawRowRaw (
            Row row, // Row (0-15)
            // Below: Characters to copy to the 5-character long row. 1 is treated like a normal character.
            char c0, char c1, char c2, char c3, char c4
        ){
            this.text[(int)row, 0] = c0;
            this.text[(int)row, 1] = c1;
            this.text[(int)row, 2] = c2;
            this.text[(int)row, 3] = c3;
            this.text[(int)row, 4] = c4;
        }
        
        public void DrawRowError (
            Row row // Row (0-15)
        ){
            this.DrawRowRaw (row, 'E', 'R', 'R', 'O', 'R');
        }
    }
    
    #endregion
    
    #region Music class, cont.
    
    static partial class Music {
        
        #region Functions (almost all small, inline) for note encoding
        
        static Row noteheadRow (
            byte byte1 // Bits 1-8 of note encoding. Bits 3-6 are relevant here.
            // Returns row (1-15) notehead should be on, or 0 if note is a rest
            // (in which case no part of the rest should affect the 0th row).
        ){
            return (Row)((byte1 & 0b111100) >> 2);
        }
        
        static bool isRest (
            byte byte1 // Bits 1-8 of note encoding. Bits 3-6 are relevant here.
        ){
            return noteheadRow (byte1) == Row.Text;
        }
        
        static bool noteIsDoubleWhole (
            byte byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
        ){
            return (byte2 & 0b1111) == 2;
        }
        
        static bool noteIsWhole (
            byte byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
        ){
            return (byte2 & 0b1111) == 3;
        }
        
        static bool noteHasStem (
            byte byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
        ){
            return (byte2 & 0b1111) >= 4;
        }

        static bool noteheadIsFilled (
            byte byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
        ){
            return (byte2 & 0b1111) >= 5;
        }

        static bool noteHasBeams (
            byte byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
        ){
            return (byte2 & 0b1111) >= 8;
        }

        static bool noteHasBeamsRight (
            byte byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
        ){
            return (byte2 & 0b1111) >= 8 && (byte2 & 0b1111) < 12;
        }
        
        static byte countNoteFlags (
            byte byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
        ){
            return (((int)byte2 & 0b1111) == 6) ? (byte)1 : (((int)byte2 & 0b1111) == 7) ? (byte)2 : (byte)0;
        }

        static byte countNoteBeams (
            byte byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
            // Returns number of beams a note has. Beamed eighth notes have one; beamed sixteenth notes have two.
        ){
            return noteHasBeams (byte2) ? (byte)(1 + ((int)byte2 & 0b1)) : (byte)0;
        }

        static byte countNoteBeamsRight (
            byte byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
            // Returns number of right-side beams a note has.
        ){
            return noteHasBeamsRight (byte2) ? (byte)(1 + (byte2 & 0b1)) : (byte)0;
        }
        
        static bool noteHasLongStem (
            byte byte2 // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
        ){
            return ((0b1100110000000000 >> ((int)byte2 & 0b1111)) & 0b1) == 1;
        }

        static sbyte noteOrientation (
            byte byte1 // Bits  1-8 of note encoding. Bits 3-6 are relevant here.
            // Returns 1 or -1. If 1, the stem (if it exists) is on top and the articulation (if it exists) is on the bottom.
        ){
            return (noteheadRow (byte1) <= Row.MdB) ? (sbyte)1 : (sbyte)(-1);
        }

        static bool noteheadIsDotted (
            byte byte2 // Bits 9-16 of note encoding. Bit 13 is relevant here.
        ){
            return (byte2 & 0b10000) > 0;
        }

        static bool noteheadTiedToNext (
            byte byte2 // Bits 9-16 of note encoding. Bit 14 is relevant here.
        ){
            return (byte2 & 0b100000) > 0;
        }
        
        #endregion
        
        #region Larger functions for note encoding

        static readonly char?[] ACCIDENTAL_CHARS = { null, 'b', '~', '#' }; // Existing/none, flat, natural, sharp

        static char? preNoteheadCharacter (
            byte byte1,   // Bits 1-8 of note encoding. Bits 3-8 are relevant here.
            bool prevTied // Whether the previous note is tied to this one.
            // Returns the character to use left of the notehead, or null if existing
            // character should be used.
        ){
            return prevTied ? '_' : ACCIDENTAL_CHARS[byte1 >> 6]; // Index is bits 7-8
        }
        
        
        static readonly char?[] ARTICULATION_CHARS = { null, '.', '>', '=' }; // Existing/none, staccato, accent, tenuto

        static char? articulationNoteheadCharacter (
            byte byte2  // Bits 9-16 of note encoding. Bits 15-16 are relevant here.
            // Returns the character to use above/below the notehead, or null if existing
            // character should be used.
        ){
            return ARTICULATION_CHARS[byte2 >> 6]; // Index is bits 15-16
        }


        static char? postNoteheadCharacter (
            byte byte2 // Bits 9-16 of note encoding. Bits 13-14 are relevant here.
            // Returns the character to use left of the notehead, or null if existing
            // character should be used.
        ){
            return noteheadIsDotted (byte2) ? '.' : noteheadTiedToNext (byte2) ? '_' : null;
        }
        
        #endregion
        
        #region Other inline functions
        
        static bool rowBesideMidB (
            Row row // Row (0-15)
        ){
            return (row == Row.LoA) || (row == Row.HiC);
        }

        static bool rowIsEdge (
            Row row // Row (0-15)
        ){
            return (row == Row.LoE) || (row == Row.HiF);
        }

        static bool rowIsSpace (
            Row row // Row (0-15)
            // Returns 1 if row is a space (odd numbered; rows that could have ledger lines don't count).
        ){
            return (int)row % 2 == 1;
        }
        
        #endregion
        
        #region Larger misc. function
        
        static byte stemHeight (
            byte byte1, // Bits 1-8 of note encoding. Bits 3-6 are relevant here.
            byte byte2  // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
            // Returns the number of rows the stem itself occupies, not including the notehead, and not including a possible
            // additional row used by a beam if such beam exists and the note has a positive orientation.
        ){
            if (!noteHasStem (byte2)) {
                return 0; // Whole or double-whole
            }
            if (!noteHasBeams (byte2)) {
                return 2; // Quarter, flagged eighth, or flagged sixteenth
            }
            // Beamed eighth or beamed sixteenth:
            bool  noteheadOnSpace = rowIsSpace (noteheadRow (byte1));
            sbyte orientation = noteOrientation (byte1);
            bool  hasLongStem = noteHasLongStem (byte2);
            return (byte)((noteheadOnSpace ? 3 : 2) + ((orientation > 0) ? 0 : 1) + (hasLongStem ? 2 : 0));
        }
        
        #endregion
        
        #region "Draw" functions
        
        static readonly string DYNAMICS_CHARACTERS = "E\0 <>.cdefmprsEE";

        static void drawDynamicsTextRow (
            Noteblock noteblock, // Noteblock in which to draw.
            byte      byte1,     // Bits 1-8 of dynamics text encoding. Bits 1-4 should be 1000.
            byte      byte2,     // Bits 9-16 of dynamics text encoding.
            byte      byte3      // Bits 17-24 of dynamics text encoding.
        ){
            int charBits;
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
            noteblock.DrawRowRaw (Row.Text, c0, c1, c2, c3, c4);
        }
        
        
        static void drawStaff (
            Noteblock noteblock,      // Noteblock in which to draw.
            byte      width,          // Width of noteblock. If less than 5, remaining column(s) will be filled with '\0's.
            byte      ledgerLineFlags // 1st bit: Whether bottom (low C) ledger line is needed.
                                      // 2nd bit: Whether top (high A) ledger line is needed.
        ){
            char[] cs = new char[5]; // Space characters - ' ' for columns < width, '\0' for columns >= width
            char[] cL = new char[5]; // Line characters  - '-' for columns < width, '\0' for columns >= width
            byte i;
            for (i = 0; i < Noteblock.WIDTH; ++i) {
                cs[i] = (i < width) ? ' ' : '\0';
                cL[i] = (i < width) ? '-' : '\0';
            }
            ushort areRowsLines = // Bit string of length 16. 1 means line, 0 means space. High B (left) to Text (right).
                (ledgerLineFlags == 0) ? (ushort)0b0001010101010000 :
                (ledgerLineFlags == 1) ? (ushort)0b0001010101010100 :
                (ledgerLineFlags == 2) ? (ushort)0b0101010101010000 :
                                         (ushort)0b0101010101010100 ;
            for (i = (byte)Row.Min; i <= (byte)Row.Max; ++i) {
                if (((areRowsLines >> i) & 0b1) > 0) {
                    noteblock.DrawRowRaw ((Row)i, cL[0], cL[1], cL[2], cL[3], cL[4]);
                }
                else {
                    noteblock.DrawRowRaw ((Row)i, cs[0], cs[1], cs[2], cs[3], cs[4]);
                }
            }
        }
        
        
        static void drawBarlineRow (
            Noteblock noteblock, // Noteblock in which to draw.
            Row       row,       // Row (0-15)
            byte      byte0      // Bits 1-8 of barline encoding. Bits 5-7 are relevant here.
        ){
            switch (byte0) {
                case 0b00000100: // Single barline
                    if (rowIsEdge (row)) noteblock.DrawRow (row, null, '+', null, '\0', '\0');
                    else                 noteblock.DrawRow (row, null, '|', null, '\0', '\0');
                    break;
                case 0b00010100: // Double barline
                    if (rowIsEdge (row)) noteblock.DrawRow (row, null, '+', '+', '\0', '\0');
                    else                 noteblock.DrawRow (row, null, '|', '|', '\0', '\0');
                    break;
                case 0b00100100: // Double barline with left repeat
                    if (rowBesideMidB (row))  noteblock.DrawRow (row, null,   '0', '|', '|', null);
                    else if (rowIsEdge (row)) noteblock.DrawRow (row, null,  null, '+', '+', null);
                    else                      noteblock.DrawRow (row, null,  null, '|', '|', null);
                    break;
                case 0b00110100: // Double barline with right repeat
                    if (rowBesideMidB (row))  noteblock.DrawRow (row, null, '|', '|',   '0', null);
                    else if (rowIsEdge (row)) noteblock.DrawRow (row, null, '+', '+',  null, null);
                    else                      noteblock.DrawRow (row, null, '|', '|',  null, null);
                    break;
                case 0b01000100: // Double barline with left and right repeats
                    if (rowBesideMidB (row))  noteblock.DrawRow (row,   '0', '|', '|', '0', '\0');
                    else if (rowIsEdge (row)) noteblock.DrawRow (row,  null, '+', '+',  null, '\0');
                    else                      noteblock.DrawRow (row,  null, '|', '|',  null, '\0');
                    break;
                case 0b01010100: // Blank column (not actually a barline)
                    noteblock.DrawRow (row, null, '\0', '\0', '\0', '\0');
                    break;
                default: // Invalid
                    noteblock.DrawRowError (row);
                    break;
            }
        }
        
        
        static void drawRest (
            Noteblock noteblock, // Noteblock in which to draw.
            byte      byte2      // Bits 9-16 of note encoding. Bits 9-12 are relevant here.
        ){
            char? d = ((byte2 & 0b10000) > 0) ? '.' : null; // A dot if dotted, else 1
            switch (byte2 & 0b1111) {
                case 0b0000: case 0b0001: // 0 permanently invalid, 1 currently unused
                    noteblock.DrawRowError (Row.MdB);
                    break;
                case 0b0010: // double whole rest
                    noteblock.DrawRow (Row.HiD, null, '#', '#', '#', null);
                    noteblock.DrawRow (Row.HiC, null, '#', '#', '#',    d);
                    noteblock.DrawRow (Row.MdB, null, '#', '#', '#', null);
                    break;
                case 0b0011: // whole rest
                    noteblock.DrawRow (Row.HiD, null, '#', '#', '#', null);
                    noteblock.DrawRow (Row.HiC, null, '#', '#', '#',    d);
                    break;
                case 0b0100: // half rest
                    noteblock.DrawRow (Row.HiC, null, '#', '#', '#',    d);
                    noteblock.DrawRow (Row.MdB, null, '#', '#', '#', null);
                    break;
                case 0b0101: // quarter rest
                    noteblock.DrawRow (Row.HiD, null, null, '\\', null, null);
                    noteblock.DrawRow (Row.HiC, null, null,  '/',    d, null);
                    noteblock.DrawRow (Row.MdB, null, null, '\\', null, null);
                    noteblock.DrawRow (Row.LoA, null, null,  'C', null, null);
                    break;
                default: { // Eighth/sixteenth rests
                    if ((byte2 & 0b1) == 0) { // Even - eighth rest
                        noteblock.DrawRow (Row.HiC, null, null, null,  'O',    d);
                        noteblock.DrawRow (Row.MdB, null, null,  '/', null, null);
                    }
                    else { // Odd - sixteenth rest
                        noteblock.DrawRow (Row.HiC, null, null, null,   'O',    d);
                        noteblock.DrawRow (Row.MdB, null, null,  'O',  null, null);
                        noteblock.DrawRow (Row.LoA, null,  '/', null,  null, null);
                    }
                    break;
                }
            }
        }
        
        
        static void drawArticulation (
            Noteblock noteblock, // Noteblock in which to draw.
            byte      byte1,     // Bits 1-8 of note encoding.
            byte      byte2      // Bits 9-16 of note encoding.
        ){
            char? articulationChar = articulationNoteheadCharacter (byte2);
            if (articulationChar == null) { return; }
            Row noteheadRow = Music.noteheadRow (byte1);
            sbyte orientation = noteOrientation (byte1);
            Row articulationRow = noteheadRow - orientation;
            if (!Enum.IsDefined (typeof (Row), articulationRow)) { return; } // If row < 0 or row > High B
            noteblock.DrawRow (articulationRow, null, null, articulationChar, null, null);
        }
        
        
        static void drawNotehead (
            Noteblock noteblock, // Noteblock in which to draw.
            byte      byte1,     // Bits 1-8 of note encoding.
            byte      byte2,     // Bits 9-16 of note encoding.
            bool      prevTied   // Whether the previous note is tied to this one.
        ){
            char? preNoteheadChar, leftChar, fillChar, rightChar, postNoteheadChar; // 5 characters to draw
            Row noteheadRow = Music.noteheadRow (byte1); // Row to draw in
            if (noteHasStem (byte2)) {
                sbyte orientation = noteOrientation (byte1);
                leftChar = (orientation > 0) ? '(' : '|';
                fillChar = noteheadIsFilled (byte2) ? '@' : '_';
                rightChar = (orientation > 0) ? '|' : ')';
            }
            else if (noteIsWhole (byte2)) {
                leftChar = '('; fillChar = '_'; rightChar = ')';
            }
            else if (noteIsDoubleWhole (byte2)) {
                leftChar = '|'; fillChar = 'O'; rightChar = '|';
            }
            else { // Invalid
                noteblock.DrawRowError (noteheadRow);
                return;
            }
            preNoteheadChar = preNoteheadCharacter (byte1, prevTied); // 'b', '~', '#', or existing
            postNoteheadChar = postNoteheadCharacter (byte2); // '.', '_', or existing
            noteblock.DrawRow (noteheadRow, preNoteheadChar, leftChar, fillChar, rightChar, postNoteheadChar);
        }
        
        static void drawStemFlagsBeams (
            Noteblock noteblock,     // Noteblock in which to draw the stem.
            byte      byte1,         // Bits 1-8 of note encoding.
            byte      byte2,         // Bits 9-16 of note encoding.
            byte      countLeftBeams // How many beams (0-2) this note should have on the left, if beamed.
            // Draws a note's stem, if it has one, and flag(s) or beam(s), if it has them.
        ){
            byte stemHeight = Music.stemHeight (byte1, byte2);
            if (stemHeight == 0) { return; }
            Row noteheadRow = Music.noteheadRow (byte1);
            sbyte orientation = noteOrientation (byte1);
            Row stemTopRow = noteheadRow + (orientation * stemHeight); // "top" meaning farthest from notehead
            
            // Draw stem
            Row row = noteheadRow + orientation;
            while (true) {
                if (orientation > 0) {
                    noteblock.DrawRow (row, null, null, null, '|' , null);
                }
                else {
                    noteblock.DrawRow (row, null, '|', null, null, null);
                }
                if (row == stemTopRow) {
                    break;
                }
                row += orientation;
            }
            
            // Draw flags, if any
            byte countFlags = countNoteFlags (byte2);
            if (countFlags >= 1) {
                char? char3 = (orientation > 0) ? null : '/';
                char? char5 = (orientation > 0) ? '\\' : null;
                noteblock.DrawRow (stemTopRow, null, null, char3, null, char5);
                if (countFlags == 2) {
                    noteblock.DrawRow ((stemTopRow - orientation), null, null, char3, null, char5);
                }
            }
            
            // Draw beams, if any
            else if (noteHasBeams (byte2)) {
                byte countRightBeams = countNoteBeamsRight (byte2);
                if (countLeftBeams == 0 && countRightBeams == 0) {
                    return; // If, for example, the user accidentally has a non-beamed note followed by a left-beamed note
                }
                if (orientation > 0) {
                    // Left beam(s)
                    if (countLeftBeams >= 1) {
                        noteblock.DrawRow ((stemTopRow + 1), '_', '_', '_', null, null);
                        if (countLeftBeams == 2) {
                            noteblock.DrawRow (stemTopRow, '_', '_', '_', null, null);
                        }
                    }
                    // Possible middle beam char above stem
                    if (countLeftBeams >= 1 && countRightBeams >= 1) {
                        noteblock.DrawRow ((stemTopRow + 1), null, null, null, '_', null);
                    }
                    // Right beam(s)
                    if (countRightBeams >= 1) {
                        noteblock.DrawRow ((stemTopRow + 1), null, null, null, null, '_');
                        if (countRightBeams == 2) {
                            noteblock.DrawRow (stemTopRow, null, null, null, null, '_');
                        }
                    }
                }
                else {
                    // Left beam(s)
                    if (countLeftBeams >= 1) {
                        noteblock.DrawRow (stemTopRow, '_', null, null, null, null);
                        if (countLeftBeams == 2) {
                            noteblock.DrawRow ((stemTopRow + 1), '_', null, null, null, null);
                        }
                    }
                    // Right beam(s)
                    if (countRightBeams >= 1) {
                        noteblock.DrawRow (stemTopRow, null, null, '_', '_', '_');
                        if (countRightBeams == 2) {
                            noteblock.DrawRow ((stemTopRow + 1), null, null, '_', '_', '_');
                        }
                    }
                }
            }
        }
        
        #endregion

        #region "Make" functions
        
        static Noteblock makeNote (
            byte byte1, // Bits 1-8 of note encoding. Bits 1-2 should be 00.
            byte byte2, // Bits 9-16 of note encoding
            byte info   // Info stored between calls to parseBytes
            // Returns a new noteblock
        ){
            Noteblock noteblock = new ();
            Row noteheadRow = Music.noteheadRow (byte1);
            byte ledgerLineFlags = // 1st bit for low ledger line, 2nd bit for high ledger line
                (byte)(((noteheadRow <= Row.LoC && noteheadRow != Row.Text) ? 1 : 0) + ((Row.HiA <= noteheadRow) ? 2 : 0));
            drawStaff (noteblock, Noteblock.WIDTH, ledgerLineFlags);
            
            if (isRest (byte1)) {
                drawRest (noteblock, byte2);
            }
            else {
                drawArticulation (noteblock, byte1, byte2);
                drawNotehead (noteblock, byte1, byte2, (info & 0b100) > 0);
                drawStemFlagsBeams (noteblock, byte1, byte2, (byte)(info & 0b11));
            }
            return noteblock;
        }
        
        
        static Noteblock makeTimeSignature (
            byte byte1 // Bits 1-8 of time signature encoding. Bits 3-8 are relevant here.
        ){
            int topNum = (byte1 / 16) + 1; // In range 1-16
            int btmNum = 1 << ((byte1 / 4) % 4); // In {1,2,4,8}
            bool topIs2Digits = (topNum > 9);
            
            Noteblock noteblock = new ();
            drawStaff (noteblock, (topIs2Digits ? (byte)4 : (byte)3), 0);
            
            if (topIs2Digits) {
                noteblock.DrawRowRaw (Row.HiC, ' ', '1', (char)('0' + (topNum % 10)), ' ', '\0');
                noteblock.DrawRowRaw (Row.LoA, ' ', ' ', (char)('0' + btmNum),        ' ', '\0');
            }
            else {
                noteblock.DrawRowRaw (Row.HiC, ' ', (char)('0' + topNum), ' ', '\0', '\0');
                noteblock.DrawRowRaw (Row.LoA, ' ', (char)('0' + btmNum), ' ', '\0', '\0');
            }
            
            return noteblock;
        }
        
        
        static readonly Row[] KEY_SIGNATURE_ROWS = new Row[11]
            { Row.LoF, Row.MdB, Row.HiE, Row.LoE, Row.LoA, Row.HiD, Row.HiG, Row.LoD, Row.LoG, Row.HiC, Row.HiF };
        
        static Noteblock makeKeySignature (
            ushort bits01to16, // Bits 1-16 of key signature encoding. Bits 4-14 are relevant here.
            ushort bits17to32  // Bits 17-32 of key signature encoding. Bits 20-30 are relevant here.
        ){
            Noteblock noteblock = new ();
            drawStaff (noteblock, Noteblock.WIDTH, 0);
            
            int LAST_IDX = KEY_SIGNATURE_ROWS.Length - 1;
            int dir = ((bits01to16 & 0b100) > 0) ? -1 : 1; // Loop backwards or forwards through KEY_SIGNATURE_ROWS
            int col = 0;
            for (int i = 0; i <= LAST_IDX; ++i) {
                Row row = KEY_SIGNATURE_ROWS[(dir > 0) ? i : (LAST_IDX - i)];
                char? charToDraw =
                    ((bits01to16 & (1 << (int)row)) > 0) ? (
                        ((bits17to32 & (1 << (int)row)) > 0) ? '~' : 'b'
                    ):(
                        ((bits17to32 & (1 << (int)row)) > 0) ? '#' : null
                    );
                if (charToDraw != null) {
                    switch (col) {
                        case 0: noteblock.DrawRow (row, charToDraw, null, null, null, null); break;
                        case 1: noteblock.DrawRow (row, null, charToDraw, null, null, null); break;
                        case 2: noteblock.DrawRow (row, null, null, charToDraw, null, null); break;
                        case 3: noteblock.DrawRow (row, null, null, null, charToDraw, null); break;
                        case 4: noteblock.DrawRow (row, null, null, null, null, charToDraw); break;
                    }
                    col = (col + 1) % Noteblock.WIDTH;
                }
            }
            return noteblock;
        }
        
        
        static readonly byte[] BARLINE_NOTEBLOCK_WIDTHS = new byte[6] {3, 4, 5, 5, 4, 1};
        
        static Noteblock makeBarline (
            byte byte1 // Bits 1-8 of barline encoding. Bits 5-7 are relevant here.
        ){
            Noteblock noteblock = new ();
            byte width = (byte)(((byte1 >> 4) >= 6) ? 5 : BARLINE_NOTEBLOCK_WIDTHS[byte1 >> 4]);
            drawStaff (noteblock, width, 0);
            for (Row row = Row.LoE; row <= Row.HiF; ++row) {
                drawBarlineRow (noteblock, row, byte1);
            }
            return noteblock;
        }
        
        
        static readonly string     CLEF_TEXT_TREBLE = @"        _   / \--|-/  |/ --|-- /|  /-|_-|/| \|\|-|\_|_/--|--O_/                 ";
        static readonly string       CLEF_TEXT_BASS = @"               -__--/  \0O--|-   /0--/-- /   /----     -----                    ";
        static readonly string CLEF_TEXT_PERCUSSION = @"               -----     ----- # # -#-#- # # -----     -----                    ";
        static readonly string      CLEF_TEXT_ERROR = @"ERRORE  O  R  R  R  E  O  R  R  R  E  O  R  R  R  E  O  R  R  R  E  O  R  RERROR";

        static Noteblock makeClef (
            byte byte1 // Bits 1-8 of clef encoding. Bits 7-8 are relevant here.
        ){
            Noteblock noteblock = new();
            string clefText = 
                (byte1 == 0b00100000) ? CLEF_TEXT_TREBLE :
                (byte1 == 0b01100000) ? CLEF_TEXT_BASS :
                (byte1 == 0b10100000) ? CLEF_TEXT_PERCUSSION : CLEF_TEXT_ERROR;
            for (Row row = Row.Min; row <= Row.Max; ++row) {
                for (byte col = 0; col < Noteblock.WIDTH; ++col) {
                    noteblock.SetChar (row, col, clefText[Noteblock.WIDTH * (Noteblock.HEIGHT - (int)row - 1) + col]);
                }
            }
            return noteblock;
        }

        #endregion

        #region High-level functions dealing with multiple noteblocks
        
        static int countNoteblocks (
            Noteblock? noteblock // This noteblock and following will be counted.
        ){
            int count = 0;
            while (noteblock != null) {
                noteblock = noteblock.Next;
                ++count;
            }
            return count;
        }
        
        
        enum ParseResult { ParsedNoteblock, ParsedAll, UnexpectedTerminator, UnexpectedEnd, InvalidByte, InternalError }

        static ParseResult parseBytes (
            byte[]         bytes,     // Array of bytes (0-terminated) from which to read.
            ref int        index,     // Reference to index in array of bytes. Calling this method usually increases it.
            ref Noteblock? noteblock, // Reference to current noteblock (or null if none). If this method creates a new
                // noteblock, noteblock will be set to it afterwards. If a terminator is parsed, noteblock will be set
                // to null.
            ref byte       info      // Reference to info stored between calls to this function.
                // Bits 1-2 - number of beams (0-2) most recent note in this measure had, or 0 if no previous note in measure.
                // Bit 3 - whether most recent note (not necessarily in measure) had a tie on its right.
                // Bit 4 - whether previous noteblock was dynamics text.
            // Returns one of the ParseResults.
        ){
            byte byte1, byte2 = 0, byte3, byte4; // Rarely all four used
            Noteblock? newNoteblock = null;
            
            // Enum-like constants local to this function, used only for maintaining *pInfo
            const byte BARLINE = 0, DYNAMICS = 1, NOTE = 2, OTHER = 3;
            byte type = OTHER; // By default, unless otherwise assigned
            
            if (index >= bytes.Length) { return ParseResult.UnexpectedEnd; }
            byte1 = bytes[index]; ++index;

            switch (byte1 & 0b11) {
                case 0b00: switch (byte1 & 0b1111) {
                    case 0b0000: switch (byte1 & 0b111111) {
                        case 0b000000: switch (byte1) {
                            case 0: // Terminator, 1 byte
                                noteblock = null;
                                return ParseResult.ParsedAll;
                            default: // Shouldn't happen
                                return ParseResult.InvalidByte;
                        }
                        case 0b100000: // Clef, 1 byte
                            newNoteblock = makeClef (byte1);
                            break;
                        default: // Shouldn't happen
                            return ParseResult.InvalidByte;
                    } break;
                    case 0b0100: // Barline, 1 byte
                        newNoteblock = makeBarline (byte1);
                        type = BARLINE;
                        break;
                    case 0b1000: // Dynamics text, 3 bytes
                        if (
                            noteblock == null      // Can't have dynamics text without a preceding noteblock
                            || (info & 0b1000) > 0 // Can't have dynamics text twice consecutively
                        ){
                            return ParseResult.InvalidByte;
                        }
                        if (index + 1 >= bytes.Length) { return ParseResult.UnexpectedEnd; }
                        byte2 = bytes[index]; ++index; if (byte2 == 0) { return ParseResult.UnexpectedTerminator; }
                        byte3 = bytes[index]; ++index; if (byte3 == 0) { return ParseResult.UnexpectedTerminator; }
                        drawDynamicsTextRow (noteblock, byte1, byte2, byte3);
                        type = DYNAMICS;
                        break;
                    default: // 0b1100, shouldn't happen
                        return ParseResult.InvalidByte;
                } break;
                case 0b01: // Note, 2 bytes
                    if (index >= bytes.Length) { return ParseResult.UnexpectedEnd; }
                    byte2 = bytes[index]; ++index; if (byte2 == 0) { return ParseResult.UnexpectedTerminator; }
                    newNoteblock = makeNote (byte1, byte2, info);
                    type = NOTE;
                    break;
                case 0b10: // Time signature, 1 byte
                    newNoteblock = makeTimeSignature (byte1);
                    break;
                case 0b11: // Key signature, 4 bytes
                    if (index + 2 >= bytes.Length) { return ParseResult.UnexpectedEnd; }
                    byte2 = bytes[index]; ++index; if (byte2 == 0) { return ParseResult.UnexpectedTerminator; }
                    byte3 = bytes[index]; ++index; if (byte3 == 0) { return ParseResult.UnexpectedTerminator; }
                    byte4 = bytes[index]; ++index; if (byte4 == 0) { return ParseResult.UnexpectedTerminator; }
                    ushort bits01to16 = (ushort)((byte2 << 8) + byte1);
                    ushort bits17to32 = (ushort)((byte4 << 8) + byte3);
                    newNoteblock = makeKeySignature (bits01to16, bits17to32);
                    break;
            }
            
            if (newNoteblock != null) { // Unless it was dynamics text, a new noteblock was created.
                if (noteblock != null) { noteblock.Next = newNoteblock; }
                noteblock = newNoteblock;
            }
            info = (byte)(
                  ((type == BARLINE) ? 0 : (type == NOTE) ? countNoteBeams (byte2) : (info & 0b11)) // bits 1-2
                + ((type == NOTE) ? (noteheadTiedToNext (byte2) ? 0b100 : 0) : (info & 0b100)) // bit 3
                + ((type == DYNAMICS) ? 0b1000 : 0)); // bit 4 
            return ParseResult.ParsedNoteblock;
        }
        
        
        static ParseResult parseBytesStartToEnd (
            byte[]         bytes,          // Array of bytes (0b11111111-terminated) from which to read.
            out Noteblock? firstNoteblock, // Will be set to first noteblock in list.
            out int        errIndex        // If an error occurs, will be set to its index in bytes array, otherwise to -1.
            // Returns one of the ParseResults
        ){
            int index = 0;
            byte info = 0;
            firstNoteblock = null;
            
            ParseResult parseResult = parseBytes (bytes, ref index, ref firstNoteblock, ref info);
            Noteblock? noteblock = firstNoteblock;
            while (parseResult == ParseResult.ParsedNoteblock) {
                parseResult = parseBytes (bytes, ref index, ref noteblock, ref info);
            }
            errIndex = (parseResult == ParseResult.ParsedAll) ? -1 : index - 1;
            return parseResult;
        }
        
        #endregion
        
        #region String conversion functions

        static void appendStaffRowInitial (
            Noteblock      staffHead,     // First noteblock in current staff.
            out Noteblock? staffHeadNext, // Will be set to first noteblock in next staff, or null if currently in last staff.
            Row            row,           // Row (0-15).
            StringBuilder  str,           // StringBuilder to which to append text of row.
            int            maxStaffWidth  // Max number of characters in the staff's string representation (not including newline).
        ){
            int limitStrLength = str.Length + maxStaffWidth;
            int unsafeStrLength = limitStrLength - Noteblock.WIDTH;
            Noteblock? currentNoteblock = staffHead;
            while (currentNoteblock != null) {
                char c0 = currentNoteblock.GetChar (row, 0);
                char c1 = currentNoteblock.GetChar (row, 1);
                char c2 = currentNoteblock.GetChar (row, 2);
                char c3 = currentNoteblock.GetChar (row, 3);
                char c4 = currentNoteblock.GetChar (row, 4);
                if (str.Length >= unsafeStrLength) {
                    int width = (c0 != '\0' ? 1 : 0) + (c1 != '\0' ? 1 : 0) + (c2 != '\0' ? 1 : 0) + (c3 != '\0' ? 1 : 0) + (c4 != '\0' ? 1 : 0);
                    if (str.Length + width + 1>= limitStrLength) { break; }
                }
                if (c0 != '\0') { str.Append (c0); }
                if (c1 != '\0') { str.Append (c1); }
                if (c2 != '\0') { str.Append (c2); }
                if (c3 != '\0') { str.Append (c3); }
                if (c4 != '\0') { str.Append (c4); }
                currentNoteblock = currentNoteblock.Next;
            }
            str.Append ('\n');
            staffHeadNext = currentNoteblock;
        }
        
        
        static void appendStaffRowSubsequent (
            Noteblock     staffHead,     // First noteblock in current staff.
            Noteblock?    staffHeadNext, // First noteblock in next staff, or null if currently in last staff.
            Row           row,           // Row (0-15).
            StringBuilder str            // StringBuilder to which to append text of row.
        ){
            Noteblock? currentNoteblock = staffHead;
            while (currentNoteblock != staffHeadNext && currentNoteblock != null) {
                char c0 = currentNoteblock.GetChar (row, 0);
                char c1 = currentNoteblock.GetChar (row, 1);
                char c2 = currentNoteblock.GetChar (row, 2);
                char c3 = currentNoteblock.GetChar (row, 3);
                char c4 = currentNoteblock.GetChar (row, 4);
                if (c0 != '\0') { str.Append (c0); }
                if (c1 != '\0') { str.Append (c1); }
                if (c2 != '\0') { str.Append (c2); }
                if (c3 != '\0') { str.Append (c3); }
                if (c4 != '\0') { str.Append (c4); }
                currentNoteblock = currentNoteblock.Next;
            }
            str.Append ('\n');
        }
        
        
        static string? noteblocksToString (
            Noteblock firstNoteblock, // Initial noteblock.
            int       maxStaffWidth   // Max width of a staff in characters. Should equal or exceed Noteblock.WIDTH.
        ){
            // Allocate a string with max length we might need if every noteblock fills all 5 columns (no '\0' column)
            int countNoteblocks = Music.countNoteblocks (firstNoteblock);
            int noteblocksPerStaff = maxStaffWidth / Noteblock.WIDTH;
            int countStaves = (countNoteblocks / noteblocksPerStaff) + (countNoteblocks % noteblocksPerStaff > 0 ? 1 : 0);
            int countChars = (Noteblock.HEIGHT * Noteblock.WIDTH * countNoteblocks) // Actual noteblock text
                + ((Noteblock.HEIGHT + 1) * countStaves) + 1; // '\n's at ends of rows and single '\0' at end of string
            StringBuilder str = new (countChars);

            // Loop over staves until last noteblock processed
            Noteblock? staffHead = firstNoteblock; // First noteblock in current staff
            while (staffHead != null) {
                // Loop over rows in staff. Rows are numbered from bottom, but we're printing from top, so loop backwards.
                Row row = Row.Max;
                appendStaffRowInitial (staffHead, out Noteblock? staffHeadNext, row, str, maxStaffWidth);
                for (--row; row >= Row.Min; --row) {
                    appendStaffRowSubsequent (staffHead, staffHeadNext, row, str);
                }
                str.Append ('\n'); // Separate staves
                staffHead = staffHeadNext;
            }
            str.Append ('\0');
            if (str.Length > countChars) { return null; } // Sanity check
            return str.ToString();
        }

        #endregion

        #region Main/IO

        // Size in bytes of largest file we would try to read from.
        const int FILE_MAX_LENGTH = 99999;
        
        static void tryReadFile (
            string  filepath, // User-entered file path and name.
            string? widthStr  // User-entered string for maximum staff width (5-255 allowed).
            // Attempts to open file, decode it, and print music.
        ){
            // Find staff width, parsing widthStr if specified
            int widthInt;
            if (widthStr != null) {
                _ = int.TryParse (widthStr, out widthInt); // widthInt set to 0 if not parsable
                if (widthInt < 5 || 255 < widthInt) {
                    Console.Write ($"  Invalid width {widthStr}\n");
                    return;
                }
            }
            else {
                widthInt = int.MaxValue; // Will effectively be ignored
            }

            // Open, check, read, and close file
            FileInfo fileInfo = new (filepath);
            if (!fileInfo.Exists) {
                Console.Write ($"  Unable to open file: {filepath}\n", filepath);
                return;
            }
            long fileSizeLong = fileInfo.Length;
            if (fileSizeLong == 0) {
                Console.Write ($"  File is empty: {filepath}\n", filepath);
                return;
            }
            if (fileSizeLong > FILE_MAX_LENGTH) {
                Console.Write ($"  File is too long (>{FILE_MAX_LENGTH} bytes): {filepath}\n", filepath);
                return;
            }
            int fileSizeInt = (int)fileSizeLong;
            using FileStream fileStream = fileInfo.OpenRead ();
            using BinaryReader binaryReader = new (fileStream);
            byte[] bytes = binaryReader.ReadBytes (fileSizeInt);
			
            
            // Array to list of noteblocks
            ParseResult parseResult = parseBytesStartToEnd (bytes, out Noteblock? firstNoteblock, out int errIndex);
            if (parseResult != ParseResult.ParsedAll || firstNoteblock == null) {
                switch (parseResult) {
                    case ParseResult.InvalidByte:
                        string errByteAsHex = bytes[errIndex].ToString("X");
                        Console.Write ($"  Invalid value 0x{errByteAsHex} at byte #{errIndex}\n");
                        break;
                    case ParseResult.UnexpectedTerminator:
                        Console.Write ($"  Invalid terminator (0b00000000) at byte #{errIndex}\n");
                        break;
                    case ParseResult.UnexpectedEnd:
                        Console.Write ("  Invalid byte sequence, no terminator (0b00000000) found\n");
                        break;
                    default:
                        Console.Write ("  Internal error while parsing noteblocks\n");
                        break;
                }
                return;
            }
            
            // List of noteblocks to string
            string? str = noteblocksToString (firstNoteblock, widthInt);
            if (str == null) {
                Console.Write ("  Internal error while converting noteblocks to string\n");
                return;
            }
            Console.Write (str);
        }


        static string? strExample (
            // Returns example string printed when user uses cmd line option -v, or null on error
        ){
            ParseResult parseResult = parseBytesStartToEnd (EXAMPLE_BYTES, out Noteblock? firstNoteblock, out int errIndex);
            if (parseResult != ParseResult.ParsedAll || firstNoteblock == null) {
                Console.Write (
                    $"  Internal error: parse result {parseResult}, error index {errIndex}, {(firstNoteblock == null ? "no" : "a")} noteblock exists\n");
                return null;
            }
            return noteblocksToString (firstNoteblock, 85);
        }
        
        
        static void testPerformance (
            string countStr // String representing how many times to call str_example. Should be >= 10.
        ){
            _ = int.TryParse (countStr, out int countInt); // countInt set to 0 if not parsable
            
            if (countInt < 10) {
                if (countInt == 0) {
                    Console.Write ("  Invalid count\n");
                }
                else {
                    Console.Write ($"  Invalid count: {countStr} < 10\n");
                }
                return;
            }
            
            // The following is over-optimized for the speed of the loop.
            // In particular, the loop does direct comparison to 0 with no modulus involved.
            int tenthOfCount = countInt / 10;
            int tenthsDone = 0; // How many times we have looped count/10 times
            int i = tenthsDone + (countInt % 10); // i will count down to 0 ten times
            Console.Write ("  Done: 00%");
            Stopwatch stopwatch = new ();
            stopwatch.Start();
            while (true) {
                string? _ = strExample(); // Meat of loop
                --i;
                if (i > 0) {
                    continue;
                }
                else {
                    ++tenthsDone;
                    Console.Write (" {0}0%", tenthsDone);
                    if (tenthsDone < 10) {
                        i = tenthOfCount; // Reset for next countdown
                    }
                    else {
                        break;
                    }
                }
            }
            stopwatch.Stop();
            long seconds = stopwatch.ElapsedMilliseconds / 1000;
            Console.Write("\n  Example output constructed {0} times in <{1} seconds", countStr, 1 + seconds);
        }


        static void Main (string[] args) {
            if (args.Length == 1 && args[0] == "-e") {
                Console.Write (STR_ENCODING);
            }
            else if (args.Length == 1 && args[0] == "-v") {
                string? example = strExample();
                if (example != null) { Console.Write (example); }
            }
            else if (args.Length == 1 && args[0] == "-p") {
                Console.Write ("  Count argument required for option -p\n");
            }
            else if (args.Length == 1 && args[0] == "-h") {
                Console.Write (STR_HELP);
			}
            else if (args.Length == 1) {
                tryReadFile (args[0], null);
            }
            else if (args.Length == 2 && args[0] == "-p") {
                testPerformance (args[1]);
            }
            else if (args.Length == 2) {
                tryReadFile (args[0], args[1]);
            }
            else {
                Console.Write (STR_HELP);
            }
        }
        
        #endregion
    }
    
    #endregion
}

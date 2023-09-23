// music_fs.fs
// Translated from C (music.c) to F#
// Takes a bit string as input, converts to music notation which is printed.
// Original C program was fundamentally an exercise in squeezing high information density out of a small number of bytes.
// This translated program is an exercise is comparing the two languages' features/syntax/performance.


open System
open System.Diagnostics
open System.IO


// Command line help arguments
let str_help =
    "  music.exe prints sheet music (drawn with ASCII characters) from an encoded file. Options:\n" +
    "    music.exe -e                   Print file encoding information\n" +
    "    music.exe -v                   Print example of visual style of output\n" +
    "    music.exe <filepath>           Read a file and print music on a continuous staff\n" +
    "    music.exe <filepath> <width>   Read a file and print music with a maximum page width (min 5, max 255)\n" +
    "    music.exe -p <count>           Test performance by repeatedly constructing the example from option -v\n"


// File encoding
let str_encoding =
    "  FILE CREATION\n" +
    "  To create an encoded file, you can use the PowerShell set-content cmdlet. For example:\n" +
    "    $b=[byte[]]@(0x53,0x61,0x75,0x63,0x65)\n" +
    "    set-content encoded_song $b -encoding byte\n" +
    "  FILE ENCODING\n" +
    "  The file consists of groups of bytes that represent \"noteblocks\", which are rectangles of text 16\n" +
    "  characters high and at most 5 characters wide. At most 3 bytes represent one noteblock.\n" +
    "  In the following descriptions, we count bits from the right, the 1's place.\n" +
    "  For example, in the bit string 10010, bits 2 and 5 are the 1s; bits 1, 3, and 4 are the 0s.\n" +
    "  Terminator (1 byte):\n" +
    "    Bits 1-8: Always 00000000\n" +
    "  Note (2 bytes):\n" +
    "    Bits 1-2:   Always 01\n" +
    "    Bits 3-6:   Rest (0) or pitches B to B to B (1 to 15)\n" +
    "    Bits 7-8:   Accidentals - none (0), flat ('b') (1), natural ('~') (2), or sharp ('#') (3)\n" +
    "    Bits 9-12:\n" +
    "      Appearance - Invalid (0), unused (1), Breve (2), whole (3), half (4), quarter (5), eighth (6-14 even),\n" +
    "      sixteenth (7-15 odd). For eighths and sixteenths, there are five encodings each that indicate whether\n" +
    "      the note is flagged or beamed and, if beamed, the beam height:\n" +
    "        Flagged (6-7) VS beamed (8-15)\n" +
    "        Beamed on left only (12-15) VS on right, plus left if preceded by a right-side beam (8-11)\n" +
    "        Beam two spaces away (8-9, 12-13) VS three spaces away (10-11, 14-15)\n" +
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
    "    Bits 4-14, 20-30:\n" +
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
    "  If you ever see a capital 'E' or the string \"ERROR\" in the generated music, your file has invalid input.\n"

// Example song to illustrate visual style of output. Used by str_example function.
let example_bytes : byte list = [
    0b01010100uy; // Blank column
    0b00100000uy; // Treble clef
    0b00000111uy; 0b11000000uy; 0b00000111uy; 0b11110110uy; // E major (C# minor) key signature - C#, D#, F#, G#
    0b00111010uy; // 4|4 time signature
    0b00000001uy; 0b00000110uy; // Eighth rest
    0b00011001uy; 0b00001010uy; // Low G, beamed eighth (left and right, tall stem)
    0b00101000uy; 0b10111010uy; 0b00100010uy; // Dynamics text " mp  "
    0b00011101uy; 0b00011000uy; // Low A, beamed eighth (left and right), dotted
    0b00011001uy; 0b00011010uy; // Low G, beamed eighth (left and right, tall stem), dotted
    0b00011101uy; 0b00001100uy; // Low A, beamed eighth (left only)
    0b00010001uy; 0b00000101uy; // Low E, quarter
    0b00000100uy; // Single barline
    0b00000001uy; 0b00000110uy; // Eighth rest
    0b00011001uy; 0b00001010uy; // Low G, beamed eighth (left and right, tall stem)
    0b00011101uy; 0b00011000uy; // Low A, beamed eighth (left and right), dotted
    0b00011001uy; 0b00011010uy; // Low G, beamed eighth (left and right, tall stem), dotted
    0b00011101uy; 0b00001100uy; // Low A, beamed eighth (left only)
    0b00010001uy; 0b00000101uy; // Low E, quarter
    0b00000100uy; // Single barline
    0b00000001uy; 0b00000110uy; // Eighth rest
    0b00011001uy; 0b00001010uy; // Low G, beamed eighth (left and right, tall stem)
    0b00011101uy; 0b00011000uy; // Low A, beamed eighth (left and right), dotted
    0b00011001uy; 0b00011010uy; // Low G, beamed eighth (left and right, tall stem), dotted
    0b00011101uy; 0b00001100uy; // Low A, beamed eighth (left only)
    0b00010001uy; 0b00000101uy; // Low E, quarter
    0b00000100uy; // Single barline
    0b00000001uy; 0b00000110uy; // Eighth rest
    0b00011001uy; 0b00001010uy; // Low G, beamed eighth (left and right, tall stem)
    0b00101000uy; 0b00110010uy; 0b00110011uy; // Dynamics text "  <<<"
    0b00011101uy; 0b00011000uy; // Low A, beamed eighth (left and right), dotted
    0b00111000uy; 0b00110011uy; 0b00110011uy; // Dynamics text "<<<<<"
    0b00011001uy; 0b00011010uy; // Low G, beamed eighth (left and right, tall stem), dotted
    0b00111000uy; 0b00110011uy; 0b00110011uy; // Dynamics text "<<<<<"
    0b00011101uy; 0b00001100uy; // Low A, beamed eighth (left only)
    0b00111000uy; 0b00110011uy; 0b00110011uy; // Dynamics text "<<<<<"
    0b00010001uy; 0b00000110uy; // Low E, lone eighth
    0b00111000uy; 0b00110011uy; 0b00110011uy; // Dynamics text "<<<<<"
    0b00100101uy; 0b01001001uy; // High C, beamed sixteenth (left and right), staccato
    0b00111000uy; 0b10010010uy; 0b00010010uy; // Dynamics text "< f  "
    0b00100101uy; 0b01101101uy; // High C, beamed sixteenth (left only), tied to next, staccato
    0b00100100uy; // Left repeat barline :|
    0b00100101uy; 0b00100100uy; // High C, half, tied to nothing
    0b00000000uy  // Terminator
]

// The following width splits the string representation of the above bytes neatly into two staves.
let example_width = 85


// ================================================================================================

// Rows (some unused, but defining all)
type Row =
| Max = 15 // Max = HiB
| HiB = 15
| HiA = 14
| HiG = 13
| HiF = 12
| HiE = 11
| HiD = 10
| HiC =  9
| MdB =  8
| LoA =  7
| LoG =  6
| LoF =  5
| LoE =  4
| LoD =  3
| LoC =  2
| LoB =  1
| Text = 0
| Min  = 0 // Min = Text


// Noteblock: Structure for text used by a note, time signature, key signature, barline, etc.
// Text is stored as a char[16,5]. If a column should not be displayed, that column's chars should
// be set to char 0 to reduce the row width. Each row in a noteblock should have the same width.
// In the C and C# versions, it also contained a pointer to make a linked list; in this version,
// we store noteblocks in an F# list instead.

type Noteblock = char[,]

module Noteblock =
    let Rows : Row list = [for row in (int Row.Min)..(int Row.Max) -> enum row]
    let Cols : int list = [for col in 0..4 -> col]
    let Height = Rows.Length // 16
    let Width  = Cols.Length //  5
    let Create () : Noteblock = Array2D.zeroCreate Height Width
    type RowInput = (char * char * char * char * char) // Input to functions that set rows


// Small functions for note encoding

let notehead_row (byte1 : byte) : Row =
    // Bits 1-8 of note encoding. Bits 3-6 are relevant here.
    (int byte1 &&& 0b111100) >>> 2 |> enum

let is_rest (byte1 : byte) : bool =
    // Bits 1-8 of note encoding. Bits 3-6 are relevant here.
    notehead_row byte1 |> int = 0

let note_appearance_nybl (byte2 : byte) : int =
    // byte2 - Bits 9-16 of note encoding.
    // Returns bits 9-12, which determine the appearance/existence of the notehead, stem, and beams or flags.
    int byte2 &&& 0b1111

let note_is_double_whole (byte2 : byte) : bool =
    // byte2 - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    note_appearance_nybl byte2 = 2

let note_is_whole (byte2 : byte) : bool =
    // byte2 - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    note_appearance_nybl byte2 = 3

let note_has_stem (byte2 : byte) : bool =
    // byte2 - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    note_appearance_nybl byte2 >= 4

let notehead_is_filled (byte2 : byte) : bool =
    // byte2 - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    note_appearance_nybl byte2 >= 5

let note_has_beams (byte2 : byte) : bool =
    // byte2 - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    note_appearance_nybl byte2 >= 8

let note_has_beams_right (byte2 : byte) : bool =
    // byte2 - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    let nybl = note_appearance_nybl byte2
    8 <= nybl && nybl < 12

let count_note_flags (byte2 : byte) : int =
    // byte2 - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    let nybl = note_appearance_nybl byte2
    if nybl = 6 then 1 else if nybl = 7 then 2 else 0

let count_note_beams (byte2 : byte) : int =
    // byte2 - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    // Returns number of beams a note has. Beamed eighth notes have one; beamed sixteenth notes have two.
    if note_has_beams byte2 then 1 + (int byte2 &&& 0b1) else 0

let count_note_beams_right (byte2 : byte) : int =
    // byte2 - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    // Returns number of right-side beams a note has.
    if note_has_beams_right byte2 then 1 + (int byte2 &&& 0b1) else 0

let note_has_long_stem (byte2 : byte) : bool =
    // byte2 - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    (0b1100110000000000 >>> note_appearance_nybl byte2) &&& 0b1 = 0b1;

let note_orientation (byte1 : byte) : int =
    // byte1 - Bits 1-8 of note encoding. Bits 3-6 are relevant here.
    if notehead_row byte1 <= Row.MdB then 1 else -1

let notehead_is_dotted (byte2 : byte) : bool =
    // byte2 - Bits 9-16 of note encoding. Bit 13 is relevant here.
    int byte2 &&& 0b10000 > 0

let notehead_tied_to_next (byte2 : byte) : bool =
    // byte2 - Bits 9-16 of note encoding. Bit 14 is relevant here.
    int byte2 &&& 0b100000 > 0


// Larger functions for note encoding

let accidental_chars = [char 1; 'b'; '~'; '#'] // Existing/none, flat, natural, sharp

let pre_notehead_character (byte1 : byte) (prevTied : bool) : char =
    // byte1    - Bits 1-8 of note encoding. Bits 3-8 are relevant here.
    // prevTied - Whether the previous note is tied to this one.
    // Returns the character to use left of the notehead, or ASCII character 1 if existing
    // character should be used.
    if prevTied then '_' else accidental_chars.[int byte1 >>> 6]

let articulation_chars = [char 1; '.'; '>'; '='] // Existing/none, staccato, accent, tenuto

let articulation_notehead_character (byte2 : byte) : char =
    // byte2 - Bits 9-16 of note encoding. Bits 15-16 are relevant here.
    // Returns the character to use above/below the notehead, or ASCII character 1 if existing
    // character should be used.
    articulation_chars.[int byte2 >>> 6]

let post_notehead_character (byte2 : byte) : char =
    // byte2 - Bits 9-16 of note encoding. Bits 13-14 are relevant here.
    // Returns the character to use left of the notehead, or ASCII character 1 if existing
    // character should be used.
    if notehead_is_dotted byte2 then '.' else if notehead_tied_to_next byte2 then '_' else char 1


// Other small functions

let row_beside_mid_B (row : Row) : bool =
    // row - Row number (0-15)
    (row = Row.LoA) || (row = Row.HiC)

let row_is_edge (row : Row) : bool =
    // Row number (0-15)
    (row = Row.LoE) || (row = Row.HiF)

let row_is_space (row : Row) : bool =
    // row - Row number (0-15)
    // Returns 1 if row is a space (odd numbered; rows that could have ledger lines don't count).
    int row % 2 = 1


// Larger misc. function

let stem_height (byte1 : byte) (byte2 : byte) : int =
    // byte1 - Bits 1-8 of note encoding. Bits 3-6 are relevant here.
    // byte2 - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    // Returns the number of rows the stem itself occupies, not including the notehead, and not including a possible
    // additional row used by a beam if such beam exists and the note has a positive orientation.
    if not <| note_has_stem byte2 then 0
    else if not <| note_has_beams byte2 then 2
    else
        let noteheadOnSpace = notehead_row byte1 |> row_is_space
        let orientation = note_orientation byte1
        let hasLongStem = note_has_long_stem byte2
        (if noteheadOnSpace then 3 else 2) + (if orientation < 0 then 1 else 0) + (if hasLongStem then 2 else 0)


// "Draw row" functions

let draw_row (noteblock : Noteblock) (row : Row) ((c0, c1, c2, c3, c4) : Noteblock.RowInput) : unit =
    // text - A noteblock's 2D array of text.
    // row  - Row to draw in.
    // c0, c1, c2, c3, c4 - Characters to copy to the 5-character long row. Pass char 1 to keep existing char.
    if c0 <> char 1 then noteblock.[int row, 0] <- c0
    if c1 <> char 1 then noteblock.[int row, 1] <- c1
    if c2 <> char 1 then noteblock.[int row, 2] <- c2
    if c3 <> char 1 then noteblock.[int row, 3] <- c3
    if c4 <> char 1 then noteblock.[int row, 4] <- c4


let draw_row_raw (noteblock : Noteblock) (row : Row) ((c0, c1, c2, c3, c4) : Noteblock.RowInput) : unit =
    // text - A noteblock's 2D array of text.
    // row  - Row to draw in.
    // c0, c1, c2, c3, c4 - Characters to copy to the 5-character long row. Char 1 is treated as a normal character.
    let n, r = noteblock, int row
    n.[r,0] <- c0; n.[r,1] <- c1; n.[r,2] <- c2; n.[r,3] <- c3; n.[r,4] <- c4


let draw_row_error (noteblock : Noteblock) (row : Row) : unit =
    // text - A noteblock's 2D array of text.
    // row  - Row to draw in.
    // Draws "ERROR" in a noteblock's row.
    draw_row_raw noteblock row ('E', 'R', 'R', 'O', 'R')


// "Draw" functions

type LedgerLineFlags = {
    NeedsHiA : bool // Whether we need the high A ledger line
    NeedsLoC : bool // Whether we need the low C ledger line
}

let ledgerLinesNeither : LedgerLineFlags = { NeedsHiA = false; NeedsLoC = false }

let draw_staff (noteblock : Noteblock) (width : int) (ledgerLineFlags : LedgerLineFlags) : unit =
    // width - Width of noteblock. If less than 5, remaining column(s) will be filled with '\0's.
    // ledgerLineFlags - Flags indicating which ledger lines we need
    
    // Space characters - ' ' for columns < width, '\0' for columns >= width
    let cs = Noteblock.Cols |> List.map (fun col -> if col < width then ' ' else char 0)
    let drawSpaceRow row = draw_row_raw noteblock row (cs.[0], cs.[1], cs.[2], cs.[3], cs.[4])

    // Line characters  - '-' for columns < width, '\0' for columns >= width
    let cL = Noteblock.Cols |> List.map (fun col -> if col < width then '-' else char 0)
    let drawLineRow row = draw_row_raw noteblock row (cL.[0], cL.[1], cL.[2], cL.[3], cL.[4])

    // Bit string of length 16. 1 means line, 0 means space. High B (left) to Text (right).
    let areRowsLines : uint16 =
        match ledgerLineFlags with
        | { NeedsHiA = false; NeedsLoC = false } -> 0b0001010101010000us
        | { NeedsHiA = false; NeedsLoC =  true } -> 0b0001010101010100us
        | { NeedsHiA =  true; NeedsLoC = false } -> 0b0101010101010000us
        | { NeedsHiA =  true; NeedsLoC =  true } -> 0b0101010101010100us

    let drawRow (row : Row) : unit =
        if (int areRowsLines >>> (int row) &&& 0b1 = 1) then drawLineRow row else drawSpaceRow row
    List.map drawRow Noteblock.Rows |> ignore
    ()
    

let dynamics_chars = ['E'; char 0; ' '; '<'; '>'; '.'; 'c'; 'd'; 'e'; 'f'; 'm'; 'p'; 'r'; 's'; 'E'; 'E']

let draw_dynamics_text_row (noteblock : Noteblock) (byte1 : byte) (byte2 : byte) (byte3 : byte) : unit =
    // noteblock - The noteblock in which to draw.
    // byte1 - Bits 1-8 of dynamics text encoding. Bits 1-4 should be 1000.
    // byte2 - Bits 9-16 of dynamics text encoding.
    // byte3 - Bits 17-24 of dynamics text encoding.
    let bitStrs = [byte1 / 16uy; byte2 % 16uy; byte2 / 16uy; byte3 % 16uy; byte3 / 16uy]
    let chars = bitStrs |> List.map (fun b -> dynamics_chars.[int b])
    draw_row_raw noteblock Row.Text (chars.[0], chars.[1], chars.[2], chars.[3], chars.[4])
    ()


let draw_barline_row (noteblock : Noteblock) (row : Row) (byte1 : byte) : unit =
    // noteblock - The noteblock in which to draw
    // row   - Row number (0-15)
    // byte1 - Bits 1-8 of barline encoding. Bits 4-7 are relevant here.
    let draw = draw_row noteblock row
    match byte1 with
    | 0b00000100uy -> // Single barline
        if row_is_edge row then draw (char 1, '+', char 1, char 0, char 0)
                            else draw (char 1, '|', char 1, char 0, char 0)
    | 0b00010100uy -> // Double barline
        if row_is_edge row then draw (char 1, '+', '+', char 0, char 0)
                            else draw (char 1, '|', '|', char 0, char 0)
    | 0b00100100uy -> // Double barline with left repeat
        if row_beside_mid_B row then draw (char 1,    '0', '|', '|', char 1)
        else if row_is_edge row then draw (char 1, char 1, '+', '+', char 1)
                                else draw (char 1, char 1, '|', '|', char 1)
    | 0b00110100uy -> // Double barline with right repeat
        if row_beside_mid_B row then draw (char 1, '|', '|',    '0', char 1)
        else if row_is_edge row then draw (char 1, '+', '+', char 1, char 1)
                                else draw (char 1, '|', '|', char 1, char 1)
    | 0b01000100uy -> // Double barline with left and right repeats
        if row_beside_mid_B row then draw (   '0', '|', '|',    '0', char 1)
        else if row_is_edge row then draw (char 1, '+', '+', char 1, char 1)
                                else draw (char 1, '|', '|', char 1, char 1)
    | 0b01010100uy -> // Blank column (not actually a barline)
        draw (char 1, char 0, char 0, char 0, char 0)
    | _ -> draw_row_error noteblock row // Invalid
    ()


let draw_rest (noteblock : Noteblock) (byte2 : byte) : unit =
    // noteblock - The noteblock in which to draw.
    // byte2 - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
    let dot = if notehead_is_dotted byte2 then '.' else char 1
    match (byte2 &&& 0b1111uy) with
    | 0b0000uy -> // 0 permanently invalid
        draw_row_error noteblock Row.MdB
    | 0b0001uy -> // 1 currently unused
        draw_row_error noteblock Row.MdB
    | 0b0010uy -> // Double whole rest
        draw_row noteblock Row.HiD (char 1, '#', '#', '#', char 1)
        draw_row noteblock Row.HiC (char 1, '#', '#', '#', dot)
        draw_row noteblock Row.MdB (char 1, '#', '#', '#', char 1)
    | 0b0011uy -> // Whole rest
        draw_row noteblock Row.HiD (char 1, '#', '#', '#', char 1)
        draw_row noteblock Row.HiC (char 1, '#', '#', '#', dot)
    | 0b0100uy -> // Half rest
        draw_row noteblock Row.HiC (char 1, '#', '#', '#', dot)
        draw_row noteblock Row.MdB (char 1, '#', '#', '#', char 1)
    | 0b0101uy -> // Quarter rest
        draw_row noteblock Row.HiD (char 1, char 1, '\\', char 1, char 1)
        draw_row noteblock Row.HiC (char 1, char 1, '/', dot, char 1)
        draw_row noteblock Row.MdB (char 1, char 1, '\\', char 1, char 1)
        draw_row noteblock Row.LoA (char 1, char 1, 'C', char 1, char 1)
    | _ -> // Eighth/sixteenth rests
        if byte2 &&& 0b1uy = 0uy then // Even - eighth rest
            draw_row noteblock Row.HiC (char 1, char 1, char 1, 'O', dot)
            draw_row noteblock Row.MdB (char 1, char 1, '/', char 1, char 1)
        else // Odd - sixteenth rest
            draw_row noteblock Row.HiC (char 1, char 1, char 1, 'O', dot)
            draw_row noteblock Row.MdB (char 1, char 1, 'O', char 1, char 1)
            draw_row noteblock Row.LoA (char 1, '/', char 1, char 1, char 1)
    ()


let draw_articulation (noteblock : Noteblock) (byte1 : byte) (byte2 : byte) : unit =
    let articulationChar = articulation_notehead_character byte2
    if articulationChar = char 1 then () else
        let noteheadRow = notehead_row byte1 |> int
        let orientation = note_orientation byte1
        let articulationRow = noteheadRow - orientation
        if articulationRow > int Row.HiB then () else
            draw_row noteblock (enum articulationRow) (char 1, char 1, articulationChar, char 1, char 1)
    ()


let draw_notehead (noteblock : Noteblock) (byte1 : byte) (byte2 : byte) (prevTied : bool) : unit =
    // noteblock - The noteblock in which to draw.
    // byte1 - Bits 1-8 of note encoding.
    // byte2 - Bits 9-16 of note encoding.
    // prevTied - Whether the previous note is tied to this one.
    // Draws a 3-character notehead, as well as the characters to the left and right, for 5 total.
    let noteheadRow = notehead_row byte1
    let noteheadChars : (char * char * char) option =
        if note_has_stem byte2 then
            let orientation = note_orientation byte1
            let leftChar = if orientation > 0 then '(' else '|'
            let fillChar = if notehead_is_filled byte2 then '@' else '_'
            let rightChar = if orientation > 0 then '|' else ')'
            Some (leftChar, fillChar, rightChar)
        else if note_is_whole byte2 then
            Some ('(', '_', ')')
        else if note_is_double_whole byte2 then
            Some ('|', 'O', '|')
        else None // Invalid
    match noteheadChars with
    | None -> 
        draw_row_error noteblock noteheadRow
    | Some(leftChar, fillChar, rightChar) ->
        let preNoteheadChar = pre_notehead_character byte1 prevTied // 'b', '~', '#', or existing
        let postNoteheadChar = post_notehead_character byte2 // '.', '_', or existing
        draw_row noteblock noteheadRow (preNoteheadChar, leftChar, fillChar, rightChar, postNoteheadChar)
    ()


let draw_stem_flags_beams (noteblock : Noteblock) (byte1 : byte) (byte2 : byte) (countLeftBeams : int) : unit =
    // noteblock - The noteblock in which to draw.
    // byte1 - Bits 1-8 of note encoding.
    // byte2 - Bits 9-16 of note encoding.
    // countLeftBeams - How many beams (0-2) this note should have on the left, if beamed.
    // Draws a note's stem, if it has one, and flag(s) or beam(s), if it has them.
    let stemHeight = stem_height byte1 byte2
    if stemHeight = 0 then ()
    else

    let noteheadRow = notehead_row byte1
    let orientation = note_orientation byte1
    let stemBtmRow, stemTopRow = // "bottom"/"top" meaning closest/farthest to/from notehead
        enum<Row> <| int noteheadRow + orientation,
        enum<Row> <| int noteheadRow + (orientation * stemHeight)
    
    // Draw stem
    let stemRows : Row list = [for i in (int stemBtmRow)..orientation..(int stemTopRow) -> enum i]
    let drawStem (row : Row) : unit =
        if orientation > 0
            then draw_row noteblock row (char 1, char 1, char 1, '|', char 1)
            else draw_row noteblock row (char 1, '|', char 1, char 1, char 1)
    List.map drawStem stemRows |> ignore

    // Draw flags, if any
    let countFlags = count_note_flags byte2
    if countFlags >= 1 then
        let flagChar = if orientation > 0 then '\\' else '/'
        let drawFlag row = draw_row noteblock row (char 1, char 1, char 1, char 1, flagChar)
        drawFlag stemTopRow 
        if countFlags = 2 then drawFlag (int stemTopRow - 1 |> enum)

    // Draw beams, if any
    else if note_has_beams byte2 then
        let countRightBeams = count_note_beams_right byte2
        if countLeftBeams = 0 && countRightBeams = 0 then
            () // If, for example, the user accidentally has a non-beamed note followed by a left-beamed note
        else
            let rowAboveStemTopRow : Row = int stemTopRow + 1 |> enum
            if orientation > 0 then
                let drawLeftBeam row = draw_row noteblock row ('_', '_', '_', char 1, char 1)
                let drawMiddleBeam row = draw_row noteblock row (char 1, char 1, char 1, '_', char 1)
                let drawRightBeam row = draw_row noteblock row (char 1, char 1, char 1, char 1, '_')
                // Left beam(s)
                if countLeftBeams >= 1 then
                    drawLeftBeam rowAboveStemTopRow
                    if countLeftBeams = 2 then drawLeftBeam stemTopRow
                // Possible middle beam char above stem
                if countLeftBeams >= 1 && countRightBeams >= 1 then
                    drawMiddleBeam rowAboveStemTopRow
                // Right beams
                if countRightBeams >= 1 then
                    drawRightBeam rowAboveStemTopRow
                    if countRightBeams = 2 then drawRightBeam stemTopRow
            else
                let drawLeftBeam row = draw_row noteblock row ('_', char 1, char 1, char 1, char 1)
                let drawRightBeam row = draw_row noteblock row (char 1, char 1, '_', '_', '_')
                // Left beam(s)
                if countLeftBeams >= 1 then
                    drawLeftBeam stemTopRow
                    if countLeftBeams = 2 then drawLeftBeam rowAboveStemTopRow
                // Right beam(s)
                if countRightBeams >= 1 then
                    drawRightBeam stemTopRow
                    if countRightBeams = 2 then drawRightBeam rowAboveStemTopRow
    () 


// "Make" functions

// Info stored between calls to parse_bytes and used by several "make" functions
type ParseInfo = {
    CountLeftBeams: int  // How many beams (0-2) this note should have on the left, if beamed.
    PrevWasTied : bool   // Whether most recent note (not necessarily in current measure) had a tie on its right.
    PrevWasDynTxt : bool // Whether previous noteblock was dynamics text.
}

let new_parse_info (countLeftBeams : int) (prevWasTied : bool) (prevWasDynTxt : bool) : ParseInfo =
    // Make a new ParseInfo record - separate function for conciseness
    {CountLeftBeams = countLeftBeams; PrevWasTied = prevWasTied; PrevWasDynTxt = prevWasDynTxt}


let make_note (byte1 : byte) (byte2 : byte) (info : ParseInfo) : Noteblock =
    // byte1 - Bits 1-8 of note encoding. Bits 1-2 should be 00.
    // byte2 - Bits 9-16 of note encoding
    // info - Info stored between calls to parse_bytes
    // Returns the new noteblock
    let noteblock = Noteblock.Create ()
    let noteheadRow = notehead_row byte1
    let ledgerLineFlags : LedgerLineFlags =
        { NeedsHiA = Row.HiA <= noteheadRow; NeedsLoC = noteheadRow <= Row.LoC && noteheadRow <> Row.Text }
    draw_staff noteblock Noteblock.Width ledgerLineFlags
    if is_rest byte1 then draw_rest noteblock byte2 else
        draw_articulation noteblock byte1 byte2
        draw_notehead noteblock byte1 byte2 info.PrevWasTied
        draw_stem_flags_beams noteblock byte1 byte2 info.CountLeftBeams
    noteblock


let make_time_signature (byte1 : byte) : Noteblock =
    // byte1 - Bits 1-8 of time signature encoding. Bits 3-8 are relevant here.
    // Returns the new noteblock
    let topNum = (int byte1 / 16) + 1 // In range 1-16
    let bottomNum = 1 <<< ((int byte1 / 4) % 4)  // In {1,2,4,8}
    let topIs2Digits = (topNum > 9)

    let noteblock = Noteblock.Create ()
    draw_staff noteblock (if topIs2Digits then 4 else 3) ledgerLinesNeither
    
    let topChars =
        let topNumRtChar = int '0' + topNum % 10 |> char
        if topIs2Digits then (' ', '1', topNumRtChar, ' ', char 0)
                        else (' ', topNumRtChar, ' ', char 0, char 0)
    let btmChars =
        let btmNumChar = int '0' + bottomNum |> char
        if topIs2Digits then (' ', ' ', btmNumChar, ' ', char 0)
                        else (' ', btmNumChar, ' ', char 0, char 0)
    draw_row_raw noteblock Row.HiC topChars
    draw_row_raw noteblock Row.LoA btmChars
    noteblock


let key_signature_rows =
    [Row.LoF; Row.MdB; Row.HiE; Row.LoE; Row.LoA; Row.HiD; Row.HiG; Row.LoD; Row.LoG; Row.HiC; Row.HiF]

let make_key_signature (bits01to16 : uint16) (bits17to32 : uint16) : Noteblock =
    // bits01to16 - Bits  1-16 of key signature encoding. Bits 4-14 are relevant here.
    // bits17to32 - Bits 17-32 of key signature encoding. Bits 20-30 are relevant here.
    let noteblock = Noteblock.Create ()
    draw_staff noteblock Noteblock.Width ledgerLinesNeither
    let rowsOrdered = // For looping through them backward or forward
        if int bits01to16 &&& 0b100 = 0 then key_signature_rows else List.rev key_signature_rows
    let rec loop (rowsRemaining : Row list) (col : int) : unit =
        match rowsRemaining with
        | [] -> ()
        | row :: rowsTail ->
            let rowBit1 = int bits01to16 &&& (1 <<< int row) > 0
            let rowBit2 = int bits17to32 &&& (1 <<< int row) > 0
            let charToDraw =
                match (rowBit1, rowBit2) with
                |  true,  true -> '~'
                |  true, false -> 'b'
                | false,  true -> '#'
                | false, false -> char 1
            if charToDraw = char 1 then
                loop rowsTail col
            else
                noteblock.[int row, col] <- charToDraw
                let newCol = (col + 1) % Noteblock.Width
                loop rowsTail newCol
    loop rowsOrdered 0
    noteblock


let barline_noteblock_widths = [3; 4; 5; 5; 4; 1]
let barline_rows : Row list = [for row in (int Row.LoE)..(int Row.HiF) do yield enum row]

let make_barline (byte1 : byte) : Noteblock =
    // byte1 - Bits 1-8 of barline encoding. Bits 5-7 are relevant here.
    let noteblock = Noteblock.Create()
    let index = int byte1 >>> 4
    let width = if index < barline_noteblock_widths.Length then barline_noteblock_widths.[index] else Noteblock.Width
    draw_staff noteblock width ledgerLinesNeither
    ignore <| List.map (fun row -> draw_barline_row noteblock row byte1) barline_rows
    noteblock


let clef_text_treble     = "        _   / \\--|-/  |/ --|-- /|  /-|_-|/| \\|\\|-|\\_|_/--|--O_/                 "
let clef_text_bass       = "               -__--/  \\0O--|-   /0--/-- /   /----     -----                    "
let clef_text_percussion = "               -----     ----- # # -#-#- # # -----     -----                    "
let clef_text_error      = "ERRORE  O  R  R  R  E  O  R  R  R  E  O  R  R  R  E  O  R  R  R  E  O  R  RERROR"

let make_clef (byte1 : byte) : Noteblock =
    // byte1 - Bits 1-8 of clef encoding. Bits 7-8 are relevant here.
    let noteblock = Noteblock.Create ()
    draw_staff noteblock Noteblock.Width ledgerLinesNeither
    let clefText =
        match byte1 with
        | 0b00100000uy -> clef_text_treble
        | 0b01100000uy -> clef_text_bass
        | 0b10100000uy -> clef_text_percussion
        | ____________ -> clef_text_error
    let drawClefChar (row : Row) (col : int) : unit =
        let index = Noteblock.Width * (Noteblock.Height - (int row) - 1) + col
        noteblock.[int row, col] <- clefText.[index]
    let drawClefRow row = List.map (drawClefChar row) Noteblock.Cols
    ignore <| List.map drawClefRow Noteblock.Rows
    noteblock


// High-level functions handling multiple noteblocks

// Status returned from parse_bytes and parse_bytes_start_to_end
type ParseResult =
| ParsedNoteblock
| ParsedAll
| UnexpectedTerminator
| UnexpectedEnd
| InvalidByte
| InternalError


let parse_bytes (bytesRemaining : byte list) (noteblockList : Noteblock list) (info : ParseInfo) : ParseResult * byte list * Noteblock list * ParseInfo =
    // Parameters:
    //   bytesRemaining - List of bytes (char 0-terminated) that we still need to parse (should be all of them on first call).
    //   noteblockOpt - List of noteblocks already generated (empty on first call) with most recently generated as head.
    //   info - Info stored between calls to this function.
    // Returns:
    //   1.  A ParseResult.
    //   2.  The updated list of bytes we still need to parse, shorter than before unless something went wrong.
    //   3.  The updated list of noteblocks, one longer than before unless something went wrong.
    //   4.  The updated ParseInfo values.
    let prevNoteblock =
        match noteblockList with
        | [] -> None
        | (noteblock :: _) -> Some(noteblock)
    
    let (result : ParseResult), (newBytesRemaining : byte list), (newNoteblockOpt : Noteblock option), (newInfoOpt : ParseInfo option) =
        // If something goes wrong, we get a bad ParseResult, the updated byte list, and None.
        // If it goes right, we get a good ParseResult, the updated byte list, and if applicable
        // a new Noteblock and ParseInfo.
        match bytesRemaining with
        | [] -> ParseResult.UnexpectedEnd, [], None, None
        | byte1 :: bytes2toEnd ->
            match (int byte1 &&& 0b11) with
            | 0b00 ->
                match int byte1 &&& 0b1100 with
                | 0b0000 ->
                    match int byte1 &&& 0b110000 with
                    | 0b000000 ->
                        match int byte1 with
                        | 0 -> // Terminator, 1 byte
                            ParseResult.ParsedAll, bytes2toEnd, None, None
                        | _ -> // Shouldn't happen
                            ParseResult.InvalidByte, bytes2toEnd, None, None
                    | 0b100000 -> // Clef, 1 byte
                        let newNoteblock = make_clef byte1
                        let newInfo = {info with PrevWasDynTxt = false}
                        ParseResult.ParsedNoteblock, bytes2toEnd, Some(newNoteblock), Some(newInfo)
                    | ________ -> // Shouldn't happen
                        ParseResult.InvalidByte, bytes2toEnd, None, None
                | 0b0100 -> // Barline, 1 byte
                    let newNoteblock = make_barline byte1
                    let newInfo = {info with CountLeftBeams = 0; PrevWasDynTxt = false}
                    ParseResult.ParsedNoteblock, bytes2toEnd, Some(newNoteblock), Some(newInfo)
                | 0b1000 -> // Dynamics text, 3 bytes
                    if info.PrevWasDynTxt then // Can't have dynamics text twice consecutively
                        ParseResult.InvalidByte, bytes2toEnd, None, None
                    else
                        match prevNoteblock with 
                        | None -> // Can't have dynamics text without a preceding noteblock
                            ParseResult.InvalidByte, bytes2toEnd, None, None
                        | Some(noteblock) ->
                            match bytes2toEnd with
                            | 0uy :: bytes3toEnd -> ParseResult.UnexpectedTerminator, bytes3toEnd, None, None
                            | _ :: 0uy :: bytes4toEnd -> ParseResult.UnexpectedTerminator, bytes4toEnd, None, None
                            | byte2 :: byte3 :: bytes4toEnd ->
                                draw_dynamics_text_row noteblock byte1 byte2 byte3
                                let newInfo = {info with PrevWasDynTxt = true}
                                ParseResult.ParsedNoteblock, bytes4toEnd, None, Some(newInfo)
                            | _ -> ParseResult.UnexpectedEnd, [], None, None
                | ______ -> // 0b1100, shouldn't happen
                    ParseResult.InvalidByte, bytes2toEnd, None, None
            | 0b01 -> // Note, 2 bytes
                match bytes2toEnd with
                | [] -> ParseResult.UnexpectedEnd, [], None, None
                | 0uy :: bytes3toEnd -> ParseResult.UnexpectedTerminator, bytes3toEnd, None, None
                | byte2 :: bytes3toEnd ->
                    let newNoteblock = make_note byte1 byte2 info
                    let newInfo : ParseInfo = {
                        CountLeftBeams = count_note_beams byte2
                        PrevWasTied = notehead_tied_to_next byte2
                        PrevWasDynTxt = false
                    }
                    ParseResult.ParsedNoteblock, bytes3toEnd, Some(newNoteblock), Some(newInfo)
            | 0b10 -> // Time signature, 1 bytes
                let newNoteblock = make_time_signature byte1
                let newInfo = {info with PrevWasDynTxt = false}
                ParseResult.ParsedNoteblock, bytes2toEnd, Some(newNoteblock), Some(newInfo)
            | 0b11 -> // Key signature, 4 bytes
                match bytes2toEnd with
                | 0uy :: bytes3toEnd -> ParseResult.UnexpectedTerminator, bytes3toEnd, None, None
                | _ :: 0uy :: bytes4toEnd -> ParseResult.UnexpectedTerminator, bytes4toEnd, None, None
                | _ :: _ :: 0uy :: bytes5toEnd -> ParseResult.UnexpectedTerminator, bytes5toEnd, None, None
                | byte2 :: byte3 :: byte4 :: bytes5toEnd ->
                    let bits01to16 = uint16 byte1 + (uint16 byte2 <<< 8)
                    let bits17to32 = uint16 byte3 + (uint16 byte4 <<< 8)
                    let newNoteblock = make_key_signature bits01to16 bits17to32
                    let newInfo = {info with PrevWasDynTxt = false}
                    ParseResult.ParsedNoteblock, bytes5toEnd, Some(newNoteblock), Some(newInfo)
                | _ -> ParseResult.UnexpectedEnd, [], None, None
            | ____ -> // If this happens then there's a programming error
                ParseResult.InternalError, bytes2toEnd, None, None
    
    let newNoteblockList =
        match newNoteblockOpt with
        | None -> noteblockList
        | Some(newNoteblock) -> newNoteblock :: noteblockList
    let newInfo =
        match newInfoOpt with
        | None -> info // Will be ignored so just use this
        | Some(newInfo) -> newInfo
    
    result, newBytesRemaining, newNoteblockList, newInfo


let parse_bytes_start_to_end (bytes : byte list) : ParseResult * Noteblock list * int option =
    // Parameters:
    //   bytes - List of bytes (char 0-terminated) to read from.
    // Returns:
    //   1.  A ParseResult
    //   2.  A generated list of noteblocks, ordered from first to last
    //   3.  If an error occured, its index in bytes, otherwise None
    let rec loop (parseResult : ParseResult, bytesRemaining : byte list, noteblockList : Noteblock list, info : ParseInfo) =
        match parseResult with
        | ParseResult.ParsedNoteblock -> loop <| parse_bytes bytesRemaining noteblockList info
        | _ -> parseResult, bytesRemaining, noteblockList, info
    let lastParseResult, bytesUnparsed, noteblockListEndToStart, _ =
        loop (ParseResult.ParsedNoteblock, bytes, [], (new_parse_info 0 false false))
    let noteblockListStartToEnd = List.rev noteblockListEndToStart
    let errIndexOpt : int option =
        if lastParseResult = ParseResult.ParsedAll then None else
            let firstUnparsedByteIndex = bytes.Length - bytesUnparsed.Length
            Some(firstUnparsedByteIndex - 1)
    lastParseResult, noteblockListStartToEnd, errIndexOpt


// String conversion functions

let append_staff_row_initial (listFromCurStaff : Noteblock list) (row : Row) (chars : char list) (maxStaffWidth : int) : Noteblock list * char list =
    // Parameters:
    //   fromCurStaff - List starting from first noteblock in current staff.
    //   row - Current row.
    //   chars - List of characters already generated.
    //   maxStaffWidth - Max number of characters in the staff's string representation (not including newline).
    // Returns:
    //   1.  Updated chars list with the additional characters from this row of the current staff.
    //   2.  List starting with the first noteblock in the next staff, or [] if we're in the last staff.
    let empty c = (c = char 0)
    let limitStrLen = chars.Length + maxStaffWidth
    let unsafeStrLen = limitStrLen - Noteblock.Width
    let rec loop (noteblockList : Noteblock list, chars : char list) =
        match noteblockList with
        | [] -> [], chars
        | head :: tail ->
            let getCh col = head.[int row, col]
            let c0, c1, c2, c3, c4 = getCh 0, getCh 1, getCh 2, getCh 3, getCh 4
            let stop =
                if chars.Length >= unsafeStrLen then
                    let flags = [c0; c1; c2; c3; c4] |> List.map (fun c -> if empty c then 0 else 1)
                    let width = List.reduce (fun a b -> a + b) flags
                    (chars.Length + width + 1) >= limitStrLen
                else false
            if not stop then
                let chs0 = if c0 = char 0 then chars else c0 :: chars
                let chs1 = if c1 = char 0 then chs0 else c1 :: chs0
                let chs2 = if c2 = char 0 then chs1 else c2 :: chs1
                let chs3 = if c3 = char 0 then chs2 else c3 :: chs2
                let chs4 = if c4 = char 0 then chs3 else c4 :: chs3
                loop (tail, chs4)
            else (noteblockList, chars)
    let listFromNextStaff, newChars = loop (listFromCurStaff, chars)
    listFromNextStaff, ('\n' :: newChars)


let append_staff_row_subsequent (listFromCurStaff : Noteblock list) (listFromNextStaff : Noteblock list) (row : Row) (chars : char list) : char list =
    // Parameters:
    //   listFromCurStaff  - List starting from first noteblock in current staff.
    //   listFromNextStaff - List starting from first noteblock in next staff, or [] if we're in the last staff.
    //   row - Current row.
    //   chars - List of characters already generated, with most recent first.
    // Returns:
    //   Updated chars list with the additional characters from this row of the current staff.
    // Note:
    //   This function is very similar to append_staff_row_initial. Because these two functions use
    //   a large portion of the program's time, I decided to separate them to optimize performance.
    let empty c = (c = char 0)
    let rec loop (noteblockList : Noteblock list, chars : char list) =
        if noteblockList = listFromNextStaff then listFromNextStaff, chars else
        match noteblockList with
        | [] -> [], chars
        | head :: tail ->
            let getCh col = head.[int row, col]
            let c0, c1, c2, c3, c4 = getCh 0, getCh 1, getCh 2, getCh 3, getCh 4
            let chs0 = if empty c0 then chars else c0 :: chars
            let chs1 = if empty c1 then chs0 else c1 :: chs0
            let chs2 = if empty c2 then chs1 else c2 :: chs1
            let chs3 = if empty c3 then chs2 else c3 :: chs2
            let chs4 = if empty c4 then chs3 else c4 :: chs3
            loop (tail, chs4)
    let _, newChars = loop (listFromCurStaff, chars)
    ('\n' :: newChars)


let noteblocks_to_string (noteblocks : Noteblock list) (maxStaffWidth : int) : string =
    // noteblocks - List of noteblocks to convert.
    // maxStaffWidth - Max width of one staff in characters. Should equal or exceed Noteblock.Width.
    // Returns the string representation of the noteblocks.
    let rec loopSubsequentRows (listFromCurStaff : Noteblock list) (listFromNextStaff : Noteblock list) (row : Row) (chars : char list) : char list=
        // Recursive function to loop through Row.HiA and lower rows in one staff
        // when we have already found listFromNextStaff while handling Row.HiB.
        if int row < int Row.Min then chars else
            let newChars = append_staff_row_subsequent listFromCurStaff listFromNextStaff row chars
            let newRow : Row = int row - 1 |> enum
            loopSubsequentRows listFromCurStaff listFromNextStaff newRow newChars
    let rec loopStaves (listFromCurStaff : Noteblock list, chars : char list) : char list =
        // Recursive function to loop over staves. If maxStaffWidth is high enough, there will be only one staff.
        if listFromCurStaff = [] then chars else 
            let listFromNextStaff, charsWithTopRow = append_staff_row_initial listFromCurStaff Row.HiB chars maxStaffWidth
            let charsWithThisStaff = loopSubsequentRows listFromCurStaff listFromNextStaff Row.HiA charsWithTopRow
            loopStaves (listFromNextStaff, charsWithThisStaff)
    let charsLastToFirst = loopStaves (noteblocks, [])
    let str = new string (charsLastToFirst |> List.toArray |> Array.rev)
    str


// Main/IO

let try_read_width_int_from_str (widthStrOpt : string option) : int option =
    // widthStrOpt - User-entered string for max width in characters of one staff
    //   (allowed: 5-255 or omitted).
    // Returns the max staff width to use while reading and printing from the file,
    //   or prints an error message and returns None.
    match widthStrOpt with
    | None -> Some(Int32.MaxValue) // Default value large enough to be effectively ignored
    | Some(widthStr) ->
        let mutable widthInt = 0
        let ok = Int32.TryParse (widthStr, &widthInt)
        if ok && (widthInt >= 5) && (widthInt <= 255) then Some(widthInt)
        else printfn "  Invalid width %s" widthStr; None


// Size in bytes of largest file we would try to read from.
let file_size_max : int = 99999

let try_read_bytes_from_file (filepath : string) : byte list =
    // filepath - User-entered file path and name.
    // Returns a list of bytes from the file,
    //   or prints an error message and returns an empty list.
    let fileInfo = FileInfo (filepath)
    let fileSizeOpt : int option =
        if not fileInfo.Exists then
            printfn "  Unable to open file: %s" filepath
            None
        else if fileInfo.Length = 0L then
            printfn "  File is empty: %s" filepath
            None
        else if fileInfo.Length > (int64 file_size_max) then
            printfn "  File is too long (>%d bytes): %s" file_size_max filepath
            None
        else Some(int fileInfo.Length)
    let bytes : byte[] =
        match fileSizeOpt with
        | None -> [||]
        | Some(fileSize) ->
            use fileStream = fileInfo.OpenRead ()
            use binaryReader = new BinaryReader (fileStream)
            binaryReader.ReadBytes (fileSize)
    Array.toList bytes


let try_get_noteblocks_from_bytes (bytes : byte list) : Noteblock list =
    // bytes - List of bytes from the file.
    // Returns a list of noteblocks generated from the bytes,
    //   or prints an error message and returns an empty list.
    let parseResult, noteblocks, errIndexOpt = parse_bytes_start_to_end bytes
    match parseResult with
    | ParsedAll ->
        noteblocks
    | InvalidByte ->
        match errIndexOpt with
        | None -> printfn "  Invalid value found"
        | Some (errIndex) ->
            if errIndex >= bytes.Length then
                printfn "  Invalid value found"
            else printfn "  Invalid value 0x%X at byte #%d" bytes.[errIndex] errIndex
        []
    | UnexpectedTerminator ->
        match errIndexOpt with
        | None -> printfn "  Invalid terminator (0b00000000) found"
        | Some(errIndex) -> printfn "  Invalid terminator (0b00000000) at byte #%d" errIndex
        []
    | UnexpectedEnd ->
        printfn "  Invalid byte sequence, no terminator (0b00000000) found"
        []
    | _ ->
        printfn "  Internal error while parsing noteblocks"
        []


let try_read_file (filepath : string) (widthStrOpt : string option) : unit =
    // filepath - User-entered file path and name.
    // widthStrOpt - User-entered string for maximum staff width, or None if not entered
    //   (5-255 allowed if it was entered).
    // Attempts to open the file, decode it, and print the music;
    //   otherwise, prints an error message.
    let widthIntOpt = try_read_width_int_from_str widthStrOpt
    match widthIntOpt with
    | None -> ()
    | Some width ->
        let bytes = try_read_bytes_from_file filepath
        if bytes = [] then ()
        else
        let noteblocks = try_get_noteblocks_from_bytes bytes
        if noteblocks = [] then ()
        else
        let str = noteblocks_to_string noteblocks width
        if str = "" then ()
        else
        printf "%s" str


let str_example () : string option =
    // Returns example string printed when user uses cmd line option -v, or None on error
    let parseResult, noteblockList, errIndexOpt = parse_bytes_start_to_end example_bytes
    if parseResult <> ParseResult.ParsedAll then
        let errIndexStr =
            match errIndexOpt with
            | Some(errIndex) -> "error index " + string errIndex
            | None -> "unknown error index"
        let nbArticle = if noteblockList = [] then "no" else "a"
        printfn "  Internal error: parse result %A, error index %s, %s noteblock exists"
            parseResult errIndexStr nbArticle
        None
    else
        let str = noteblocks_to_string noteblockList example_width
        Some(str)


let test_performance (countStr : string) : unit =
    // countStr - String representing how many times to call str_example. Should be >= 10.
    let mutable countInt = 0
    let ok = Int32.TryParse (countStr, &countInt)
    let countInt = countInt // back to immutable
    if not ok then
        printfn "  Invalid count: not a whole number"
    else if countInt < 10 then
        printfn "  Invalid count: %s < 10" countStr
    else // rest of function
    
    // The following is over-optimized for the speed of the loop.
    // The loop could be written as a recursive function but is not.
    // Also, the loop does direct comparison to 0 with no modulus involved.
    let tenthOfCount = countInt / 10
    let mutable tenthsDone = 0 // How many times we have looped count/10 times 
    let mutable i = tenthOfCount + (countInt % 10) // i will count down to 0 ten times
    let mutable cont = true
    printf "  Done: 00%%"
    let stopwatch = Stopwatch ()
    stopwatch.Start()
    while cont do
        // Meat of loop
        let s = str_example ()
        ignore s
        // Rest of loop
        i <- i - 1
        if i <= 0 then
            tenthsDone <- tenthsDone + 1
            printf " %d0%%" tenthsDone
            if tenthsDone < 10 then i <- tenthOfCount // Reset
            else cont <- false // Break
    stopwatch.Stop()
    let seconds = stopwatch.ElapsedMilliseconds / 1000L
    printfn ""
    printfn "  Example output constructed %s times in <%d seconds" countStr seconds
    ()


[<EntryPoint>]
let main (args : string[]) : int =
    // args - Array of command-line argument strings
    match args.Length with
    | 1 ->
        match args.[0] with
        | "-e" ->
            printf "%s" str_encoding
        | "-v" ->
            let strExampleOpt : string option = str_example ()
            match strExampleOpt with
                | Some(strExample) -> printf "%s" strExample
                | None -> ()
        | "-p" -> printf "  Count argument required for option -p"
        | "-h" -> printf "%s" str_help
        | ____ -> try_read_file args.[0] None
    | 2 ->
        match args.[0] with
        | "-p" -> test_performance args.[1]
        | ____ -> try_read_file args.[0] <| Some(args.[1])
    | _ ->
        printf "%s" str_help
    0
    
//**************************************************************************************
// music2_general2.c
// This file contains general logic that depends on individual byte group types' logic.
//**************************************************************************************


// External inclusions
#include <limits.h> // INT_MAX, UINT_MAX
#include <stdio.h>  // printf, fopen_s
#include <stdlib.h> // malloc, atoi
#include <stddef.h> // NULL
#include <string.h> // strcmp, strcpy
#include <time.h>   // time

// Internal inclusions
#include "music2_data.h"
#include "music2_draw_note.h"
#include "music2_draw_other.h"
#include "music2_general1.h"
#include "music2_noteblock.h"


//*****************
// Byte Group Type
//*****************

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



//*************************************************
// Functions for parsing byte groups to noteblocks
//*************************************************

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
                         // function creates a new noteblock, *ppNoteblock will point to it afterwards. If a terminator
                         //  is parsed, *ppNoteblock will be set to NULL.
    unsigned int*        pParseInfo   // Pointer to info stored between calls to this function - see update_parse_info.
                         // The first time you call this function, initialize *pParseInfo = 0.
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



//***************************
// Byte to string formatting
//***************************

// Format a byte in format XXXX XXXX (no trailing char 0).
void format_byte_XXXX_XXXX (
    char          byteStr[9], // Output param, char[9] that will be set with format XXXX XXXX.
    unsigned char byte        // Byte to format.
){
    byteStr[0] = '0' + ((byte & 0b00000001) > 0);
    byteStr[1] = '0' + ((byte & 0b00000010) > 0);
    byteStr[2] = '0' + ((byte & 0b00000100) > 0);
    byteStr[3] = '0' + ((byte & 0b00001000) > 0);
    byteStr[4] = ' ';
    byteStr[5] = '0' + ((byte & 0b00010000) > 0);
    byteStr[6] = '0' + ((byte & 0b00100000) > 0);
    byteStr[7] = '0' + ((byte & 0b01000000) > 0);
    byteStr[8] = '0' + ((byte & 0b10000000) > 0);
}


// Format a byte in format 0bXXXXXXXX (with trailing char 0).
void format_byte_0b (
    char          byteStr[11], // Output param, char[11] that will be set with format 0bXXXXXXXX.
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



//*********
// Main IO
//*********

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


// Given the argument the user entered after option -v, get the array of example bytes and the staff width to use.
int get_example_bytes_width (
    char*           typeArg,        // User-entered argument after -v, or NULL if none, which results in the general
                    // example song.
    unsigned char** ppExampleBytes, // *ppExampleBytes will be set to the array of encoded bytes to parse,
                    // or NULL if arg is invalid.
    int*            pExampleWidth   // *pExampleWidth will be set to the staff width to use, or 0 if arg is invalid.
    // Returns 1 if argument is valid (and params passed correctly), otherwise 0.
){
    if (ppExampleBytes != NULL) *ppExampleBytes = NULL;
    if (pExampleWidth != NULL) *pExampleWidth = 0;
    if (ppExampleBytes == NULL || pExampleWidth == NULL) return 0;

    *ppExampleBytes =
        (typeArg == NULL || strcmp(typeArg, "") == 0) ? EXAMPLE_BYTES :
        (strcmp (typeArg, "clef") == 0) ? DTL_BYTES_CLEF :
        (strcmp (typeArg, "key") == 0) ? DTL_BYTES_KEY_CHANGE :
        (strcmp (typeArg, "time") == 0) ? DTL_BYTES_TIME_CHANGE :
        (strcmp (typeArg, "note") == 0) ? DTL_BYTES_NOTE :
        (strcmp (typeArg, "text") == 0) ? DTL_BYTES_TEXT :
        (strcmp (typeArg, "barline") == 0) ? DTL_BYTES_BARLINE :
        NULL;
    if (*ppExampleBytes == NULL) return 0;
    *pExampleWidth = (*ppExampleBytes == EXAMPLE_BYTES) ? EXAMPLE_WIDTH : DTL_WIDTH;
    return 1;
}


// Get an example string to print when user uses cmd line option -v.
// If input is invalid or an error occurs, prints error information and returns NULL.
char* str_example (
    unsigned char* pExampleBytes, // Pointer to array of encoded bytes to parse.
    int            exampleWidth   // Max width of a staff in characters.
    // Returns example string to print.
){
    // Process example bytes to a list of noteblocks
    struct noteblock* p1stNoteblock = NULL;
    int errIndex = 0;
    int parseResult = parse_bytes_start_to_end (pExampleBytes, &p1stNoteblock, &errIndex);
    if (parseResult != PARSE_RESULT_PARSED_ALL) {
        char byteStr[11];
        format_byte_from_index (byteStr, pExampleBytes, errIndex);
        char* noteblockCountStr = (p1stNoteblock == NULL) ? "no" : "at least one";
        printf ("  Internal error: parse result %d; error index %d; byte %s; %s noteblock exists\n",
            parseResult, errIndex, byteStr, noteblockCountStr);
        free_noteblocks (p1stNoteblock);
        return NULL;
    }

    // Process list of noteblocks to a single string
    char* str = noteblocks_to_string (p1stNoteblock, exampleWidth);
    free_noteblocks (p1stNoteblock);
    return str;
}


// Given the example bytes used to print an example string for cmd line option -vb,
// format the raw bytes in binary with eight bytes per line, like this:
// (XXXX XXXX, ..., XXXX XXXX,\n
//             ...            \n
//  XXXX XXXX, ..., XXXX XXXX)\n\0
char* str_format_example_bytes (
    unsigned char* pExampleBytes // Pointer to array of encoded bytes.
    // Returns formatted string representing the bytes.
){
    // Allocate a string with max length we might need. Each line has (bytes * 11 + 2) chars.
    size_t countBytes = strlen (pExampleBytes) + 1; // +1 for ending byte 0
    if (countBytes <= 1) return NULL;
    size_t countLines = (countBytes / 8) + (countBytes % 8 > 0);
    size_t countChars = (countBytes * 11) + countLines + 1; // +countLines for '\n's, +1 for char 0
    char* str = malloc (countChars);
    if (str == NULL) return NULL;

    // Format bytes into string
    size_t strIndex = 0;
    size_t byteIndex = 0;
    while (byteIndex < countBytes) {
        // Safety check, shouldn't happen.
        if (strIndex + 12 >= countChars) break;
        // Add one byte to line
        unsigned char byte = pExampleBytes[byteIndex];
        ++byteIndex;
        str[strIndex] = ' '; ++strIndex;
        char* pByteStr = &(str[strIndex]);
        format_byte_XXXX_XXXX (pByteStr, byte);
        strIndex += 9;
        str[strIndex] = ',';  ++strIndex;
        // If at end of line, add \n
        if ((byteIndex % 8 == 0) || (byteIndex >= countBytes)) {
            str[strIndex] = '\n'; ++strIndex;
        }
    }
    str[0] = '('; // Overwrite the first space
    str[strIndex - 2] = ')'; // Overwrite the most recent comma
    // Keep str[strIndex - 1] == '\n'
    str[strIndex] = '\0'; ++strIndex; // Append '\0'
    return str;
}


// Print an example string when user chooses option -v
void show_example (
    char* typeArg,  // User-entered argument after -v, or NULL if none, which results in the general example song.
    int   showBytes // Whether to show the bytes under the music notation.
){
    unsigned char* pExampleBytes = NULL;
    int exampleWidth = 0;
    int isArgValid = get_example_bytes_width (typeArg, &pExampleBytes, &exampleWidth);
    if (!isArgValid) {
        printf ("  Invalid argument \"%s\"\n", typeArg);
        return;
    }
    char* strExample = str_example (pExampleBytes, exampleWidth);
    if (strExample == NULL) return;
    char* strBytes = showBytes ? str_format_example_bytes (pExampleBytes) : NULL;
    if (showBytes && strBytes == NULL) return;
    printf (strExample);
    free (strExample);
    if (strBytes != NULL) {
        printf ("Encoding:\n%s\n\n", strBytes);
        free (strBytes);
    }
}


// Test performance by constructing str_example count times
void test_performance (
    char* countStr, // String representing how many times to call str_example. Should be >= 10.
    char* typeArg   // User-entered argument after -v, or NULL if none, which results in the general example song.
){
    // Parse countStr
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

    // Process typeArg
    unsigned char* pExampleBytes = NULL;
    int exampleWidth = 0;
    int isArgValid = get_example_bytes_width (typeArg, &pExampleBytes, &exampleWidth);
    if (!isArgValid) {
        printf ("  Invalid argument \"%s\"\n", typeArg);
        return;
    }

    // Try once to build the string, make sure there's no error.
    char* s = str_example (pExampleBytes, exampleWidth);
    if (s == NULL) return;
    free (s);

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
        s = str_example (pExampleBytes, exampleWidth);
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

    // Output
    time (&time1);
    int dur = (int)(time1 - time0);
    printf ("\n  Example output constructed %s times in <%d seconds\n", countStr, dur);
}

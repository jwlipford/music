//*****************************************************************************
// music2_general1.h.
// Scope: Private to music2 program.
// Bare-bones .h file to facilitate linking. See the .c file for descriptions.
//*****************************************************************************

#pragma once

#include "music2_noteblock.h"

const unsigned short STD_STAFF_BITSTR;
void draw_staff (char* pText, int width, unsigned short staffBitstr);
inline int row_is_beside_mid_B (int row){
    return (row == ROW_LO_A) || (row == ROW_HI_C);
}
inline int row_is_edge (int row){
    return (row == ROW_LO_E) || (row == ROW_HI_F);
}
inline int row_is_space (int row){
    return row % 2;
}

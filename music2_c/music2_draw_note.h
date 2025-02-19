//*****************************************************************************
// music2_draw_note.h.
// Scope: Private to music2 program.
// Bare-bones .h file to facilitate linking. See the .c file for descriptions.
//*****************************************************************************

#pragma once

struct noteblock* make_nn (unsigned char byte1, unsigned char byte2, unsigned int  parseInfo);
struct noteblock* make_nb (unsigned char byte1, unsigned char byte2, unsigned char byte3, unsigned int  parseInfo);

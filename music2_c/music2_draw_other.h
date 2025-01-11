//*****************************************************************************
// music2_draw_other.h.
// Scope: Private to music2 program.
// Bare-bones .h file to facilitate linking. See the .c file for descriptions.
//*****************************************************************************

#pragma once

void draw_dynamics_text_row (char* pText, unsigned char byte1, unsigned char byte2, unsigned char byte3);
struct noteblock* make_time_signature (unsigned char byte);
struct noteblock* make_key_signature (unsigned short bits01to16, unsigned short bits17to32);
struct noteblock* make_barline (unsigned char byte);
struct noteblock* make_clef (unsigned char byte);

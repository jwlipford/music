MusicM(mode,arg2,arg3) d main(mode,arg2,arg3) q
	;**************************************************************************
	; MusicM
	; Translated from C (music.c) to MUMPS.
	; Takes a bit string as input, converts to music notation which is printed.
	; Original C program was fundamentally an exercise in squeezing high information density out of a small number of bytes.
	; This translated program is an exercise is comparing the two languages' features/syntax.
	;
	; This routine is almost entirely pure ANSI M.
	; The exception is that one use of the OPEN command is written in an implementation-specific way.
	;
	; Note about performance:
	; MUMPS doesn't natively support inlined functions or even inlined constants
	; (or a lot of other stuff that programmers take for granted in other languages).
	; I experimented with inlining many of the functions and constants in this routine.
	; Doing that was a huge improvement - it halved the executation time.
	; To be clear, the routine you see before you is not the version with inlining.
	;**************************************************************************
	;
	;
	;
	;
	;
	;***********************
	; Help text and example
	;***********************
	;
	;--------
	; DESCRIPTION: Returns help text describing the command line arguments
	;--------
strHelp() ;
	n str
	s str=str_"  MusicM prints sheet music (drawn with ASCII characters) from an encoded file."_$c(13,10)
	s str=str_"  There are several ways to call the routine:"_$c(13,10)
	s str=str_"    d ^MusicM(""E"")                   Print file encoding information"_$c(13,10)
	s str=str_"    d ^MusicM(""V"")                   Print example of visual style of output"_$c(13,10)
	s str=str_"    d ^MusicM(""F"",filepath)          Read a file and print music on a continuous staff"_$c(13,10)
	s str=str_"    d ^MusicM(""F"",filepath,width)    Read a file and print music with a maximum page width (min 5, max 255)"_$c(13,10)
	s str=str_"    d ^MusicM(""P"",count)             Test performance by repeatedly constructing the example from option V"_$c(13,10)
	q str
	;
	;--------
	; DESCRIPTION: Returns help text describing the file encoding format
	;--------
strEncoding() ;
	n str
	s str=str_"  FILE CREATION"_$c(13,10)
	s str=str_"  To create an encoded file, you can use the PowerShell set-content cmdlet. For example:"_$c(13,10)
	s str=str_"    $b=[byte[]]@(0x53,0x61,0x75,0x63,0x65)"_$c(13,10)
	s str=str_"    set-content encoded_song $b -encoding byte"_$c(13,10)
	s str=str_"  FILE ENCODING"_$c(13,10)
	s str=str_"  The file consists of groups of bytes that represent ""noteblocks"", which are rectangles of text 16"_$c(13,10)
	s str=str_"  characters high and at most 5 characters wide. At most 4 bytes represent one noteblock."_$c(13,10)
	s str=str_"  In the following descriptions, we count bits from the right, the 1's place."_$c(13,10)
	s str=str_"  For example, in the bit string 10010, bits 2 and 5 are the 1s; bits 1, 3, and 4 are the 0s."_$c(13,10)
	s str=str_"  For two bytes, their bits numbered in base 32 are: 87654321 GFEDCBA9."_$c(13,10)
	s str=str_"  Terminator (1 byte):"_$c(13,10)
	s str=str_"    Bits 1-8: Always 00000000"_$c(13,10)
	s str=str_"  Note (2 bytes):"_$c(13,10)
	s str=str_"    Bits 1-2:   Always 01"_$c(13,10)
	s str=str_"    Bits 3-6:   Rest (0) or pitches low B (1) to middle B (8) to high B (15)"_$c(13,10)
	s str=str_"    Bits 7-8:   Accidentals - none (0), flat ('b') (1), natural ('~') (2), or sharp ('#') (3)"_$c(13,10)
	s str=str_"    Bits 9-12:"_$c(13,10)
	s str=str_"      Appearance - Invalid (0), unused (1), Breve (2), whole (3), half (4), quarter (5), eighth (6-14 even),"_$c(13,10)
	s str=str_"      sixteenth (7-15 odd). For eighths and sixteenths, there are five encodings each that indicate whether"_$c(13,10)
	s str=str_"      the note is flagged or beamed and, if beamed, the beam height:"_$c(13,10)
	s str=str_"        Flagged (6-7) VS beamed (8-15)."_$c(13,10)
	s str=str_"        Beamed on left only with number of left-side beams determined by previous note (12-15), VS"_$c(13,10)
	s str=str_"          beamed on right at least once with number of left-side beams determined by previous note (8-11)."_$c(13,10)
	s str=str_"        Beam two spaces away (8-9, 12-13) VS three spaces away (10-11, 14-15)."_$c(13,10)
	s str=str_"    Bit  13:    Dotted"_$c(13,10)
	s str=str_"    Bit  14:    Tie/slur after"_$c(13,10)
	s str=str_"    Bits 15-16: Articulations: None (0), staccato (1), accent (2), tenuto (3)"_$c(13,10)
	s str=str_"  Time change (1 byte):"_$c(13,10)
	s str=str_"    Bits 1-2: Always 10"_$c(13,10)
	s str=str_"    Bits 3-4: Bottom number - 1, 2, 4, or 8 (encoded as 0 to 3)"_$c(13,10)
	s str=str_"    Bits 5-8: Top number - 1 to 16 (encoded as 0 to 15)"_$c(13,10)
	s str=str_"  Key change (4 bytes):"_$c(13,10)
	s str=str_"    Bits 1-2: Always 11"_$c(13,10)
	s str=str_"    Bit  3:   Arrangement of accidentals - Resembling Db major scale (0) or B major scale (1)"_$c(13,10)
	s str=str_"    Bits 4-14, 20-30:"_$c(13,10)
	s str=str_"      Pitches go from low D (bits 4 and 20) to high G (bits 14 and 30)."_$c(13,10)
	s str=str_"      If a pitch has a 1 in the first bit string but not the second, it is flat ('b')."_$c(13,10)
	s str=str_"      If a pitch has a 1 in the second bit string but not the first, it is sharp ('#')."_$c(13,10)
	s str=str_"      If a pitch has a 1 in both bit strings, it is natural ('~')."_$c(13,10)
	s str=str_"    Bits 15-16, 17-19, 31-32:"_$c(13,10)
	s str=str_"      Use these bits to make sure each byte has at least one 1 so it's not a terminator."_$c(13,10)
	s str=str_"  Barline (1 byte):"_$c(13,10)
	s str=str_"    Bits 1-4: Always 0100"_$c(13,10)
	s str=str_"    Bit  5-7: Type of barline -"_$c(13,10)
	s str=str_"              single (0), double (1), left repeat (2), right repeat (3), both repeats (4), blank column (5)"_$c(13,10)
	s str=str_"    Bit  8:   Unused"_$c(13,10)
	s str=str_"  Dynamics text (3 bytes):"_$c(13,10)
	s str=str_"    Applies to the previous noteblock. (Non-note noteblocks might need to have part of a crescendo/"_$c(13,10)
	s str=str_"    decrescendo under them.) Invalid if this is the first byte."_$c(13,10)
	s str=str_"    Bits 1-4: Always 1000"_$c(13,10)
	s str=str_"    Bits 5-8, 9-12, 13-16, 17-20, 21-24:"_$c(13,10)
	s str=str_"      Each group of four bits represents one of these characters (encoded as 1-13, 0 invalid, 14-15 unused):"_$c(13,10)
	s str=str_"      Null, space, '<', '>', '.', 'c', 'd', 'e', 'f', 'm', 'p', 'r', 's'"_$c(13,10)
	s str=str_"      These 12 characters can make text such as "" ppp "", ""cresc"", "" decr"", "" mp<<"", ""<<f>>"", etc."_$c(13,10)
	s str=str_"  Clef (1 byte):"_$c(13,10)
	s str=str_"    Elsewhere in these descriptions, pitch names assume treble clef, but here you can draw a different clef."_$c(13,10)
	s str=str_"    Bits 1-6: Always 100000"_$c(13,10)
	s str=str_"    Bits 7-8: Type - Treble (0), bass (1), percussion (2)"_$c(13,10)
	s str=str_"  If you ever see a capital 'E' or the string ""ERROR"" in the generated music, your file has invalid input."_$c(13,10)
	q str
	;
	;--------
	; DESCRIPTION: Helper subroutine to append up to four values to an array
	; PARAMETERS:
	;  array (IN/OUT)      - Array with structure array(0)=count, array(#)=one value
	;  v1    (IN,OPTIONAL) - 1st value to append
	;  v2    (IN,OPTIONAL) - 2nd value to append
	;  v3    (IN,OPTIONAL) - 3rd value to append
	;  v4    (IN,OPTIONAL) - 4th value to append
	;--------
arrayAppend(array,v1,v2,v3,v4) ;
	q:v1=""  s array(0)=array(0)+1,array(array(0))=v1
	q:v2=""  s array(0)=array(0)+1,array(array(0))=v2
	q:v3=""  s array(0)=array(0)+1,array(array(0))=v3
	q:v4=""  s array(0)=array(0)+1,array(array(0))=v4
	q
	;
	;--------
	; DESCRIPTION: Generates an array of bytes representing an example song in the file encoding format.
	;              Decoded, the example song is a good illustration of the visual style of the output.
	;              This subroutine is used by the strExample function.
	; PARAMETERS:
	;  bytes (OUT) - Encoded example song. bytes(0)=count, bytes(#)=one byte.
	;--------
genExampleBytes(bytes) ;
	k bytes
	d arrayAppend(.bytes,84)     ;Blank column
	d arrayAppend(.bytes,32)           ;Treble clef
	d arrayAppend(.bytes,7,192,7,246)  ;E major (C# minor) key signature - C#, D#, F#, G#
	d arrayAppend(.bytes,58)     ;4|4 time signature
	d arrayAppend(.bytes,1,6)        ;Eighth rest
	d arrayAppend(.bytes,25,10)      ;Low G, beamed eighth (left and right, tall stem)
	d arrayAppend(.bytes,40,186,34)  ;Dynamics text " mp  "
	d arrayAppend(.bytes,29,24)      ;Low A, beamed eighth (left and right), dotted
	d arrayAppend(.bytes,25,26)      ;Low G, beamed eighth (left and right, tall stem), dotted
	d arrayAppend(.bytes,29,12)      ;Low A, beamed eighth (left only)
	d arrayAppend(.bytes,17,5)       ;Low E, quarter
	d arrayAppend(.bytes,4)      ;Single barline
	d arrayAppend(.bytes,1,6)        ;Eighth rest
	d arrayAppend(.bytes,25,10)      ;Low G, beamed eighth (left and right, tall stem)
	d arrayAppend(.bytes,29,24)      ;Low A, beamed eighth (left and right), dotted
	d arrayAppend(.bytes,25,26)      ;Low G, beamed eighth (left and right, tall stem), dotted
	d arrayAppend(.bytes,29,12)      ;Low A, beamed eighth (left only)
	d arrayAppend(.bytes,17,5)       ;Low E, quarter
	d arrayAppend(.bytes,4)      ;Single barline
	d arrayAppend(.bytes,1,6)        ;Eighth rest
	d arrayAppend(.bytes,25,10)      ;Low G, beamed eighth (left and right, tall stem)
	d arrayAppend(.bytes,29,24)      ;Low A, beamed eighth (left and right), dotted
	d arrayAppend(.bytes,25,26)      ;Low G, beamed eighth (left and right, tall stem), dotted
	d arrayAppend(.bytes,29,12)      ;Low A, beamed eighth (left only)
	d arrayAppend(.bytes,17,5)       ;Low E, quarter
	d arrayAppend(.bytes,4)      ;Single barline
	d arrayAppend(.bytes,1,6)        ;Eighth rest
	d arrayAppend(.bytes,25,10)      ;Low G, beamed eighth (left and right, tall stem)
	d arrayAppend(.bytes,40,50,51)   ;Dynamics text "  <<<"
	d arrayAppend(.bytes,29,24)      ;Low A, beamed eighth (left and right), dotted
	d arrayAppend(.bytes,56,51,51)   ;Dynamics text "<<<<<"
	d arrayAppend(.bytes,25,26)      ;Low G, beamed eighth (left and right, tall stem), dotted
	d arrayAppend(.bytes,56,51,51)   ;Dynamics text "<<<<<"
	d arrayAppend(.bytes,29,12)      ;Low A, beamed eighth (left only)
	d arrayAppend(.bytes,56,51,51)   ;// Dynamics text "<<<<<"
	d arrayAppend(.bytes,17,6)       ;Low E, lone eighth
	d arrayAppend(.bytes,56,51,51)   ;// Dynamics text "<<<<<"
	d arrayAppend(.bytes,37,73)      ;High C, beamed sixteenth (left and right), staccato
	d arrayAppend(.bytes,56,146,18)  ;Dynamics text "< f  "
	d arrayAppend(.bytes,37,109)     ;High C, beamed sixteenth (left only), tied to next, staccato
	d arrayAppend(.bytes,36)         ;Left repeat barline
	d arrayAppend(.bytes,37,36)      ;High C, half, tied to nothing
	d arrayAppend(.bytes,0)      ;Terminator
	q
	;
	;--------
	; DESCRIPTION: Returns 85. This width splits the string representation of the above bytes neatly into two staves.
	;--------
ExampleWidth() ;
	q 85
	;
	;
	;
	;
	;
	;**************************************************************************
	; noteblockArray structure
	;
	; The following tags have to do with or operate on the noteblockArray structure, which is structured like this:
	;   noteblockArray(0) = count of noteblocks.
	;   noteblockArray(noteblockIndex) represents one noteblock in range 1:noteblockArray(0).
	;   noteblockArray(noteblockIndex,rowIndex) = one row in the noteblock, at most 5 characters wide,
	;                                             where rowIndex is in range 0:15.
	;**************************************************************************
	;
	;--------
	; DESCRIPTION: Max width of a noteblock. $L(noteblockArray(noteblockIndex,rowIndex)) should never exceed this.
	;--------
NoteblockMaxWidth() ;
	q 5
	;--------
	; ENUM Rows (some unused, but defining all) - the 16 rows in a noteblock
	;--------
RowMax() q 15  ;Max = High B
RowHiB() q 15
RowHiA() q 14
RowHiG() q 13
RowHiF() q 12
RowHiE() q 11
RowHiD() q 10
RowHiC() q 9
RowMdB() q 8
RowLoA() q 7
RowLoG() q 6
RowLoF() q 5
RowLoE() q 4
RowLoD() q 3
RowLoC() q 2
RowLoB() q 1
RowTxt() q 0
	;
	;--------
	; DESCRIPTION: Returns the count of noteblocks in the noteblockArray
	; PARAMETERS:
	;  nbArray (IN) - The noteblockArray structure
	;--------
nbaryCount(nbArray) ;
	q nbArray(0)
	;
	;--------
	; DESCRIPTION: Append a new empty noteblock to the noteblockArray
	; PARAMETERS:
	;  nbArray (IN/OUT) - The noteblockArray structure
	;  width   (IN)     - The width of the new noteblock, should be 1-5
	; RETURNS:     The index of the new noteblock in noteblockArray
	;--------
nbaryAppend(noteblockArray,width) ;
	n nbIndex,rowIndex,rowText
	i (width<1)!(5<width) QUIT ""  ;Validity check
	s nbIndex=noteblockArray(0)+1,noteblockArray(0)=nbIndex
	s rowText=$j("",width)  ;String of `width` spaces
	f rowIndex=0:1:15 s noteblockArray(nbIndex,rowIndex)=rowText
	q nbIndex
	;
	;--------
	; DESCRIPTION: Returns one noteblockArray row
	;  nbArray (IN/OUT) - The noteblockArray structure
	;  nbIndex (IN)     - noteblock index in noteblockArray
	;  row     (IN)     - row number (0-15)
	;--------
nbaryGetRow(nbArray,nbIndex,row) ;
	q nbArray(nbIndex,row)
	;
	;--------
	; DESCRIPTION: Overwrite a noteblockArray row. $c(1) in the passed text means don't overwrite that character.
	; PARAMETERS:
	;  nbArray (IN/OUT) - The noteblockArray structure
	;  nbIndex (IN)     - noteblock index in noteblockArray
	;  row     (IN)     - row number (0-15)
	;  text    (IN)     - text whose characters excluding $c(1) will be written
	;--------
nbaryDrawRow(nbArray,nbIndex,row,text) ;
	n rowText,char
	s rowText=nbArray(nbIndex,row)
	f char=1:1:$L(rowText) s:$e(text,char)'=$c(1) $e(rowText,char)=$e(text,char)
	s nbArray(nbIndex,row)=rowText
	q
	;
	;--------
	; DESCRIPTION: Overwrite a noteblockArray row. $c(1) in the passed text is treated like a normal character.
	; PARAMETERS:
	;  nbArray (IN/OUT) - The noteblockArray structure
	;  nbIndex (IN)     - noteblock index in noteblockArray
	;  row     (IN)     - row number (0-15)
	;  text    (IN)     - text whose characters will be written
	;--------
nbaryDrawRowRaw(nbArray,nbIndex,row,text) ;
	s nbArray(nbIndex,row)=text
	q
	;
	;--------
	; DESCRIPTION: Overwrite a noteblockArray row with the string "ERROR".
	; PARAMETERS:
	;  nbArray (IN/OUT) - The noteblockArray structure
	;  nbIndex (IN)     - noteblock index in noteblockArray
	;  row     (IN)     - row number (0-15)
	;--------
nbaryDrawRowError(nbArray,nbIndex,row) ;
	s nbArray(nbIndex,row)="ERROR"
	q
	;
	;
	;
	;
	;
	;***********************************
	; Small functions for note encoding
	;***********************************
	;
	;--------
	; DESCRIPTION: Utility function to extract one or more bits from a number.
	;              For example (imagining that MUMPS supported binary literals),
	;              $$bits(0b11010010,3,5) returns 0b10100.
	; PARAMETERS:
	;  number (IN) - An integer
	;  index  (IN) - The 1-based index of a bit in the number. Bit 1 is the rightmost, least significant bit.
	;  count  (IN) - How many bits to extract, starting at index. If null, the function returns 0.
	; RETURNS:     Extracted bits (as a number, not a string)
	;--------
bits(number,index,count) ;
	q number\(2**(index-1))#(2**count)
	;
	;--------
	; DESCRIPTION: Returns the row (1-15) a note-type noteblock's notehead should go on,
	;              or 0 if it is a rest (in which case no part of the rest should affect the 0th row)
	; PARAMETERS:
	;  byte1 (IN) - Bits 1-8 of note encoding. Bits 3-6 are relevant here.
	;--------
noteheadRow(byte1) ;
	q $$bits(byte1,3,4)
	;
	;--------
	; DESCRIPTION: Whether a note-type noteblock is a rest
	; PARAMETERS:
	;  byte1 (IN) - Bits 1-8 of note encoding. Bits 3-6 are relevant here.
	;--------
isRest(byte1) ;
	q ($$noteheadRow(byte1)=0)
	;
	;--------
	; DESCRIPTION: Whether a note-type noteblock is a double whole note (AKA breve)
	; PARAMETERS:
	;  byte2 (IN) - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
	;--------
noteIsDoubleWhole(byte2) ;
	q ($$bits(byte2,1,4)=2)
	;
	;--------
	; DESCRIPTION: Whether a note-type noteblock is a whole note
	; PARAMETERS:
	;  byte2 (IN) - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
	;--------
noteIsWhole(byte2) ;
	q $$bits(byte2,1,4)=3
	;
	;--------
	; DESCRIPTION: Whether a note-type noteblock has a stem
	; PARAMETERS:
	;  byte2 (IN) - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
	;--------
noteHasStem(byte2) ;
	q $$bits(byte2,1,4)>=4
	;
	;--------
	; DESCRIPTION: Whether a note-type noteblock's notehead is filled
	; PARAMETERS:
	;  byte2 (IN) - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
	;--------
noteheadIsFilled(byte2) ;
	q $$bits(byte2,1,4)>=5
	;
	;--------
	; DESCRIPTION: Whether a note-type noteblock has beams on the stem (on either or both sides)
	; PARAMETERS:
	;  byte2 (IN) - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
	;--------
noteHasBeams(byte2) ;
	q $$bits(byte2,1,4)>=8
	;
	;--------
	; DESCRIPTION: Whether a note-type noteblock has beams on the right side of the stem
	; PARAMETERS:
	;  byte2 (IN) - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
	;--------
noteHasBeamsRight(byte2) ;
	n bits9to12
	s bits9to12=$$bits(byte2,1,4)
	q (bits9to12>=8)&(bits9to12<12)
	;
	;--------
	; DESCRIPTION: A note-type noteblock's number of flags on the stem
	; PARAMETERS:
	;  byte2 (IN) - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
	;--------
countNoteFlags(byte2) ;
	n bits9to12
	s bits9to12=$$bits(byte2,1,4)
	q $s((bits9to12=6):1,(bits9to12=7):2,1:0)
	;
	;--------
	; DESCRIPTION: A note-type noteblock's number of beams on the stem (on either or both sides).
	;              Beamed eighth notes have one, beamed sixteenth notes have two, and others have zero.
	; PARAMETERS:
	;  byte2 (IN) - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
	;--------
countNoteBeams(byte2) ;
	i $$noteHasBeams(byte2) q $$bits(byte2,1,1)+1
	q 0
	;
	;--------
	; DESCRIPTION: A note-type noteblock's number of beams on the right side of the stem.
	; PARAMETERS:
	;  byte2 (IN) - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
	;--------
countNoteBeamsRight(byte2) ;
	i $$noteHasBeamsRight(byte2) q $$bits(byte2,1,1)+1
	q 0
	;
	;--------
	; DESCRIPTION: Whether a note-type noteblock with a beam has an extra-long stem
	; PARAMETERS:
	;  byte2 (IN) - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
	;--------
noteHasLongStem(byte2) ;
	n bits9to12
	s bits9to12=$$bits(byte2,1,4)
	;For some reason, the "string contains" operator `[` used in this manner is often faster than $select
	q ",10,11,14,15"[(","_bits9to12_",")
	;
	;--------
	; DESCRIPTION: Returns 1 or -1. If 1, the stem (if it exists) is on top and the articulation (if it exists) is on the bottom.
	; PARAMETERS:
	;  byte1 (IN) - Bits 1-8 of note encoding. Bits 3-6 are relevant here.
	;--------
noteOrientation(byte1) ;
	i $$noteheadRow(byte1)<=$$RowMdB() q 1
	q -1
	;
	;--------
	; DESCRIPTION: Whether a note-type noteblock's notehead is dotted
	; PARAMETERS:
	;  byte2 (IN) - Bits 9-16 of note encoding. Bit 13 is relevant here.
	;--------
noteheadIsDotted(byte2) ;
	q $$bits(byte2,5,1)
	;
	;--------
	; DESCRIPTION: Whether a note-type noteblock's notehead is tied to the next note
	; PARAMETERS:
	;  byte2 (IN) - Bits 9-16 of note encoding. Bit 14 is relevant here.
	;--------
noteheadTiedToNext(byte2) ;
	q $$bits(byte2,6,1)
	;
	;
	;
	;
	;
	;**********************************************************
	; Slightly larger functions for interpreting encoded bytes
	;**********************************************************
	;
	;--------
	; DESCRIPTION: Returns the character to use left of the notehead, or $c(1) if existing
	;              character should be used. If not $c(1), it will be one of 'b' (flat), '~' (natural), or '#' (sharp).
	; PARAMETERS:
	;  byte1    (IN) - Bits 1-8 of note encoding. Bits 3-8 are relevant here.
	;  prevTied (IN) - Whether the previous note is tied to this one.
	;--------
preNoteheadCharacter(byte1,prevTied) ;
	n bits7to8
	i prevTied QUIT "_"
	s bits7to8=$$bits(byte1,7,2)
	q $e(($c(1)_"b~#"),(bits7to8+1))
	;
	;--------
	; DESCRIPTION: Returns the character to use above/below the notehead, or $c(1) if existing
	;              character should be used. If not $c(1), it will be one of '.' (staccato), '>' (accent), or '=' (tenuto).
	; PARAMETERS:
	;  byte2 (IN) - Bits 9-16 of note encoding. Bits 15-16 are relevant here.
	;--------
articulationNoteheadCharacter(byte2) ;
	n bits15to16
	s bits15to16=$$bits(byte2,7,2)
	q $e(($c(1)_".>="),(bits15to16+1))
	;
	;--------
	; DESCRIPTION: Returns the character to use left of the notehead, or $c(1) if existing
	;              character should be used.
	; PARAMETERS:
	;  byte2 (IN) - Bits 9-16 of note encoding. Bits 13-14 are relevant here.
	;--------
postNoteheadCharacter(byte2) ;
	i $$noteheadIsDotted(byte2) QUIT "."
	i $$noteheadTiedToNext(byte2) QUIT "_"
	q $c(1)
	;
	;
	;
	;
	;
	;***********************
	; Other misc. functions
	;***********************
	;
	;--------
	; DESCRIPTION: Whether the row is a space adjacent to the Middle B line
	; PARAMETERS:
	;  row (IN) - row number (0-15)
	;--------
rowBesideMidB(row) ;
	q (row=$$RowLoA())!(row=$$RowHiC())
	;
	;--------
	; DESCRIPTION: Whether a row is a line at the top or bottom of the staff - the Low E line or High F line
	; PARAMETERS:
	;  row (IN) - row number (0-15)
	;--------
rowIsEdge(row) ;
	q (row=$$RowLoE())!(row=$$RowHiF())
	;
	;--------
	; DESCRIPTION: Whether the row is a space (meaning odd numbered - rows that could have ledger lines don't count)
	; PARAMETERS:
	;  row (IN) - row number (0-15)
	;--------
rowIsSpace(row) ;
	q row#2
	;
	;--------
	; DESCRIPTION: Returns the number of rows the stem itself occupies, not including the notehead, and not including a possible
	;              additional row used by a beam if such beam exists and the note has a positive orientation.
	; PARAMETERS:
	;  byte1 (IN) - Bits 1-8 of note encoding. Bits 3-6 are relevant here.
	;  byte2 (IN) - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
	;--------
stemHeight(byte1,byte2) ;
	n noteheadOnSpace,orientation,hasLongStem
	i '$$noteHasStem(byte2) QUIT 0   ;Whole or double-whole note
	i '$$noteHasBeams(byte2) QUIT 2  ;Quarter, flagged eighth, or flagged sixteenth note
	;Beamed eighth or beamed sixteenth
	s noteheadOnSpace=$$rowIsSpace($$noteheadRow(byte1))
	s orientation=$$noteOrientation(byte1)
	s hasLongStem=$$noteHasLongStem(byte2)
	q (2+noteheadOnSpace)+(orientation<0)+(hasLongStem*2)
	;
	;
	;
	;
	;
	;**********************
	; "Draw" functions to draw within existing noteblocks
	;**********************
	;
	;--------
	; DESCRIPTION: Draw the staff in a noteblock. We assume the noteblock was already created by nbaryAppend,
	;              meaning it contains 16 same-width rows of spaces. This subroutine draws the lines.
	; PARAMETERS:
	;  nbArray         (IN/OUT) - the noteblockArray structure
	;  nbIndex         (IN)     - noteblock index in noteblockArray
	;  ledgerLineFlags (IN)     - 1st bit: Whether bottom (low C) ledger line is needed.
	;                             2nd bit: Whether top (high A) ledger line is needed.
	;--------
drawStaff(nbArray,nbIndex,ledgerLineFlags) ;
	n width,lineText,row
	s width=$L($$nbaryGetRow(.nbArray,nbIndex,0))
	s lineText=$e("-----",1,width)
	i ledgerLineFlags#2 d nbaryDrawRowRaw(.nbArray,nbIndex,$$RowLoC(),lineText)
	f row=$$RowLoE(),$$RowLoG(),$$RowMdB(),$$RowHiD(),$$RowHiF() d
	. d nbaryDrawRowRaw(.nbArray,nbIndex,row,lineText)
	i ledgerLineFlags\2 d nbaryDrawRowRaw(.nbArray,nbIndex,$$RowHiA(),lineText)
	q
	;
	;--------
	; DESCRIPTION: The sixteen characters that can be represented by a four-bit dynamics text encoding.
	;              Bits 0,14,15 are invalid/unused, so they are 'E' for error.
	;--------
dynamicsCharacters() ;
	q "E"_$c(0)_" <>.cdefmprsEE"
	;
	;--------
	; DESCRIPTION: Draw the dynamics text row (bottom row, 0) in a noteblock
	; PARAMETERS:
	;  nbArray (IN/OUT) - The noteblockArray structure.
	;  nbIndex (IN)     - Noteblock index in noteblockArray.
	;  byte1   (IN)     - Bits 1-8 of dynamics text encoding. Bits 1-4 should be 1000.
	;  byte2   (IN)     - Bits 9-16 of dynamics text encoding.
	;  byte3   (IN)     - Bits 17-24 of dynamics text encoding.
	;--------
drawDynamicsTextRow(nbArray,nbIndex,byte1,byte2,byte3) ;
	n chars,charBits,dynamicsCharacters
	s dynamicsCharacters=$$dynamicsCharacters()
	s charBits=$$bits(byte1,5,4)  ;Bits 5-8
	s $e(chars,1)=$e(dynamicsCharacters,charBits+1)
	s charBits=$$bits(byte2,1,4)  ;Bits 9-12
	s $e(chars,2)=$e(dynamicsCharacters,charBits+1)
	s charBits=$$bits(byte2,5,4)  ;Bits 13-16
	s $e(chars,3)=$e(dynamicsCharacters,charBits+1)
	s charBits=$$bits(byte3,1,4)  ;Bits 17-20
	s $e(chars,4)=$e(dynamicsCharacters,charBits+1)
	s charBits=$$bits(byte3,5,4)  ;Bits 21-24
	s $e(chars,5)=$e(dynamicsCharacters,charBits+1)
	d nbaryDrawRowRaw(.nbArray,nbIndex,$$RowTxt(),chars)
	q
	;
	;--------
	; DESCRIPTION: Draw one row of a barline in a noteblock
	; PARAMETERS:
	;  nbArray (IN/OUT) - The noteblockArray structure.
	;  nbIndex (IN)     - Noteblock index in noteblockArray.
	;  row     (IN)     - Row number (0-15).
	;  byte    (IN)     - Bits 1-8 of barline encoding. Bits 5-7 are relevant here.
	;--------
drawBarlineRow(nbArray,nbIndex,row,byte) ;
	n bits5to7
	s bits5to7=$$bits(byte,5,3)
	i bits5to7=0 d     ;Single barline
	. i $$rowIsEdge(row) d nbaryDrawRow(.nbArray,nbIndex,row,$c(1)_"+"_$c(1)) i 1
	. e  d nbaryDrawRow(.nbArray,nbIndex,row,$c(1)_"|"_$c(1))
	e  i bits5to7=1 d  ;Double barline
	. i $$rowIsEdge(row) d nbaryDrawRow(.nbArray,nbIndex,row,$c(1)_"++") i 1
	. e  d nbaryDrawRow(.nbArray,nbIndex,row,$c(1)_"||")
	e  i bits5to7=2 d  ;Double barline with left repeat
	. i $$rowBesideMidB(row) d nbaryDrawRow(.nbArray,nbIndex,row,$c(1)_"0||"_$c(1)) i 1
	. e  i $$rowIsEdge(row) d nbaryDrawRow(.nbArray,nbIndex,row,$c(1,1)_"++"_$c(1)) i 1
	. e  d nbaryDrawRow(.nbArray,nbIndex,row,$c(1,1)_"||"_$c(1))
	e  i bits5to7=3 d  ;Double barline with right repeat
	. i $$rowBesideMidB(row) d nbaryDrawRow(.nbArray,nbIndex,row,$c(1)_"||0"_$c(1)) i 1
	. e  i $$rowIsEdge(row) d nbaryDrawRow(.nbArray,nbIndex,row,$c(1)_"++"_$c(1,1)) i 1
	. e  d nbaryDrawRow(.nbArray,nbIndex,row,$c(1)_"||"_$c(1,1))
	e  i bits5to7=4 d  ;Double barline with left and right repeats
	. i $$rowBesideMidB(row) d nbaryDrawRow(.nbArray,nbIndex,row,"0||0") i 1
	. e  i $$rowIsEdge(row) d nbaryDrawRow(.nbArray,nbIndex,row,$c(1)_"++"_$c(1)) i 1
	. e  d nbaryDrawRow(.nbArray,nbIndex,row,$c(1)_"||"_$c(1))
	e  i bits5to7=5    ;Blank column (not actually a barline)
	; Nothing to draw for bits5to7=5
	e  d  ;Invalid
	. d nbaryDrawRowError(.nbArray,nbIndex,row)
	q
	;
	;--------
	; DESCRIPTION: Draw a rest in a noteblock
	; PARAMETERS:
	;  nbArray (IN/OUT) - The noteblockArray structure.
	;  nbIndex (IN)     - Noteblock index in noteblockArray.
	;  byte2   (IN)     - Bits 9-16 of note encoding. Bits 9-12 are relevant here.
	;--------
drawRest(nbArray,nbIndex,byte2) ;
	n dd,bits9to12
	s dd=$c(1)
	i $$noteheadIsDotted(byte2) s dd="."
	s bits9to12=$$bits(byte2,1,4)
	;
	i bits9to12<2 d     ;0 permanently invalid, 1 currently unused
	. d nbaryDrawRowError(.nbArray,nbIndex,$$RowMdB())
	e  i bits9to12=2 d  ;Double whole rest
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowHiD(),$c(1)_"###"_$c(1))
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowHiC(),$c(1)_"###"_dd)
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowMdB(),$c(1)_"###"_$c(1))
	e  i bits9to12=3 d  ;Whole rest
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowHiD(),$c(1)_"###"_$c(1))
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowHiC(),$c(1)_"###"_dd)
	e  i bits9to12=4 d  ;Half rest
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowHiC(),$c(1)_"###"_dd)
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowMdB(),$c(1)_"###"_$c(1))
	e  i bits9to12=5 d  ;Quarter rest
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowHiD(),$c(1,1)_"\"_$c(1,1))
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowHiC(),$c(1,1)_"/"_dd_$c(1))
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowMdB(),$c(1,1)_"\"_$c(1,1))
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowLoA(),$c(1,1)_"C"_$c(1,1))
	e  i (bits9to12#2)=0 d  ;Even number > 5 - eighth rest
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowHiC(),$c(1,1,1)_"O"_dd)
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowMdB(),$c(1,1)_"/"_$c(1,1))
	e  d                    ;Odd number > 5 - sixteenth rest
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowHiC(),$c(1,1,1)_"O"_dd)
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowMdB(),$c(1,1)_"O"_$c(1,1))
	. d nbaryDrawRow(.nbArray,nbIndex,$$RowLoA(),$c(1)_"/"_$c(1,1,1))
	q
	;
	;--------
	; DESCRIPTION: Draw the articulation, if one exists, below or above the notehead
	; PARAMETERS:
	;  nbArray (IN/OUT) - The noteblockArray structure.
	;  nbIndex (IN)     - Noteblock index in noteblockArray.
	;  byte1   (IN)     - Bits 1-8 of note encoding.
	;  byte2   (IN)     - Bits 9-16 of note encoding.
	;--------
drawArticulation(nbArray,nbIndex,byte1,byte2) ;
	n articulationChar,noteheadRow,orientation,articulationRow,rowText
	s articulationChar=$$articulationNoteheadCharacter(byte2)
	i articulationChar=$c(1) QUIT
	s noteheadRow=$$noteheadRow(byte1)
	s orientation=$$noteOrientation(byte1)
	s articulationRow=(noteheadRow-orientation)
	i articulationRow>$$RowMax() QUIT
	s rowText=$c(1,1)_articulationChar_$c(1,1)
	d nbaryDrawRow(.nbArray,nbIndex,articulationRow,rowText)
	q
	;
	;--------
	; DESCRIPTION: Draws a 3-character notehead, as well as the characters to the left and right, for 5 total.
	; PARAMETERS:
	;  nbArray  (IN/OUT) - The noteblockArray structure.
	;  nbIndex  (IN)     - Noteblock index in noteblockArray.
	;  byte1    (IN)     - Bits 1-8 of note encoding.
	;  byte2    (IN)     - Bits 9-16 of note encoding.
	;  prevTied (IN)     - Whether the previous note is tied to this one.
	;--------
drawNotehead(nbArray,nbIndex,byte1,byte2,prevTied) ;
	n noteheadRow,orientation,noteheadIsFilled,rowText
	;
	s noteheadRow=$$noteheadRow(byte1)
	s rowText=$c(1,1,1,1,1)
	;
	;Set characters 2-4 of rowText (main notehead)
	i $$noteHasStem(byte2) d
	. s orientation=$$noteOrientation(byte1)
	. s noteheadIsFilled=$$noteheadIsFilled(byte2)
	. s $e(rowText,2)=$s(orientation>0:"(",1:"|")
	. s $e(rowText,3)=$s(noteheadIsFilled:"@",1:"_")
	. s $e(rowText,4)=$s(orientation>0:"|",1:")")
	e  i $$noteIsWhole(byte2) d
	. s $e(rowText,2,4)="(_)"
	e  i $$noteIsDoubleWhole(byte2) d
	. s $e(rowText,2,4)="|O|"
	e  d  QUIT  ;Invalid
	. d nbaryDrawRowError(.nbArray,nbIndex,noteheadRow)
	;
	;Set characters 1 and 5 of rowText
	s $e(rowText,1)=$$preNoteheadCharacter(byte1,prevTied)  ; 'b', '~', '#', or existing
	s $e(rowText,5)=$$postNoteheadCharacter(byte2)         ; '.', '_', or existing
	;
	d nbaryDrawRow(.nbArray,nbIndex,noteheadRow,rowText)
	q
	;
	;--------
	; DESCRIPTION: Draw a note's stem, if it has one, and flag(s) or beam(s), if it has them.
	; PARAMETERS:
	;  nbArray        (IN/OUT) - The noteblockArray structure.
	;  nbIndex        (IN)     - Noteblock index in noteblockArray.
	;  byte1          (IN)     - Bits 1-8 of note encoding.
	;  byte2          (IN)     - Bits 9-16 of note encoding.
	;  countLeftBeams (IN)     - How many beams (0-2) this note should have on the left, if beamed.
	;--------
drawStemsFlagsBeams(nbArray,nbIndex,byte1,byte2,countLeftBeams) ;
	n stemHeight,noteheadRow,orientation,stemBtmRow,stemTopRow
	n row,rowText,countFlags,countRightBeams
	;
	s stemHeight=$$stemHeight(byte1,byte2)
	i stemHeight'>0 QUIT
	s noteheadRow=$$noteheadRow(byte1)
	s orientation=$$noteOrientation(byte1)
	s stemBtmRow=(noteheadRow+orientation)  ;"bottom" meaning next to notehead
	s stemTopRow=(noteheadRow+(orientation*stemHeight))  ;"top" meaning farthest from notehead
	;
	;Draw stem
	s:orientation>0 rowText=$c(1,1,1)_"|"_$c(1)
	s:orientation<0 rowText=$c(1)_"|"_$c(1,1,1)
	f row=stemBtmRow:orientation:stemTopRow d
	. d nbaryDrawRow(.nbArray,nbIndex,row,rowText)
	;
	;Draw flags, if any
	s countFlags=$$countNoteFlags(byte2)
	i countFlags>=1 d
	. s:orientation>0 rowText=$c(1,1,1,1)_"\"
	. s:orientation<0 rowText=$c(1,1)_"/"_$c(1,1)
	. d nbaryDrawRow(.nbArray,nbIndex,stemTopRow,rowText)
	. i countFlags=2 d nbaryDrawRow(.nbArray,nbIndex,(stemTopRow-orientation),rowText)
	;
	;Draw beams, if any
	e  i $$noteHasBeams(byte2) d
	. s countRightBeams=$$countNoteBeamsRight(byte2)
	. i countLeftBeams=0,countRightBeams=0 q  ;If, for example, the user accidentally has a non-beamed note followed by a left-beamed note
	. ;
	. i orientation>0 d
	. . ;Left beam(s)
	. . i countLeftBeams>=1 d
	. . . s rowText="___"_$c(1,1)
	. . . d nbaryDrawRow(.nbArray,nbIndex,(stemTopRow+1),rowText)
	. . . i countLeftBeams=2 d nbaryDrawRow(.nbArray,nbIndex,stemTopRow,rowText)
	. . ;Possible middle beam char above stem
	. . i countLeftBeams>=1,countRightBeams>=1 d
	. . . s rowText=$c(1,1,1)_"_"_$c(1)
	. . . d nbaryDrawRow(.nbArray,nbIndex,(stemTopRow+1),rowText)
	. . ;Right beams
	. . i countRightBeams>=1 d
	. . . s rowText=$c(1,1,1,1)_"_"
	. . . d nbaryDrawRow(.nbArray,nbIndex,(stemTopRow+1),rowText)
	. . . i countRightBeams=2 d nbaryDrawRow(.nbArray,nbIndex,stemTopRow,rowText)
	. e  d
	. . ;Left beams
	. . i countLeftBeams>=1 d
	. . . s rowText="_"_$c(1,1,1,1)
	. . . d nbaryDrawRow(.nbArray,nbIndex,stemTopRow,rowText)
	. . . i countLeftBeams=2 d nbaryDrawRow(.nbArray,nbIndex,(stemTopRow+1),rowText)
	. . ;Right beams
	. . i countRightBeams>=1 d
	. . . s rowText=$c(1,1)_"___"
	. . . d nbaryDrawRow(.nbArray,nbIndex,stemTopRow,rowText)
	. . . i countRightBeams=2 d nbaryDrawRow(.nbArray,nbIndex,(stemTopRow+1),rowText)
	q
	;
	;
	;
	;
	;
	;********************************************************
	; "Make" functions to make different types of noteblocks
	;********************************************************
	;
	;--------
	; DESCRIPTION: Make a note-type noteblock
	; PARAMETERS:
	;  nbArray (IN/OUT) - The noteblockArray structure.
	;  byte1   (IN)     - // Bits 1-8 of note encoding. Bits 1-2 should be 00.
	;  byte2   (IN)     - Bits 9-16 of note encoding.
	;  info    (IN)     - Array of info stored between calls to parseBytes. See that function for structure.
	; RETURNS:     Index of the new noteblock in the noteblockArray.
	;--------
makeNote(nbArray,byte1,byte2,info) ;
	n nbIndex,noteheadRow,ledgerLineFlags
	;
	s noteheadRow=$$noteheadRow(byte1)
	s ledgerLineFlags=((noteheadRow<=$$RowLoC())&(noteheadRow'=$$RowTxt()))  ;First bit for low ledger line
	s ledgerLineFlags=ledgerLineFlags+(2*($$RowHiA()<=noteheadRow))  ;Second bit for high ledger line
	;
	s nbIndex=$$nbaryAppend(.nbArray,5)
	d drawStaff(.nbArray,nbIndex,ledgerLineFlags)
	;
	i $$isRest(byte1) d
	. d drawRest(.nbArray,nbIndex,byte2)
	e  d
	. d drawArticulation(.nbArray,nbIndex,byte1,byte2)
	. d drawNotehead(.nbArray,nbIndex,byte1,byte2,info("tied"))
	. d drawStemsFlagsBeams(.nbArray,nbIndex,byte1,byte2,info("beams"))
	q nbIndex
	;
	;--------
	; DESCRIPTION: Make a time signature noteblock
	; PARAMETERS:
	;  nbArray (IN/OUT) - The noteblockArray structure.
	;  byte    (IN)     - Bits 1-8 of time signature encoding. Bits 3-8 are relevant here.
	; RETURNS:     Index of the new noteblock in the noteblockArray.
	;--------
makeTimeSignature(nbArray,byte) ;
	n nbIndex,topNum,btmNum,topIs2Digits
	;
	s topNum=$$bits(byte,5,4)+1
	s btmNum=2**$$bits(byte,3,2)
	s topIs2Digits=(topNum>9)
	;
	s nbIndex=$$nbaryAppend(.nbArray,$s(topIs2Digits:4,1:3))
	d drawStaff(.nbArray,nbIndex,0)
	;
	i topIs2Digits d
	. d nbaryDrawRowRaw(.nbArray,nbIndex,$$RowHiC()," "_topNum_" ")
	. d nbaryDrawRowRaw(.nbArray,nbIndex,$$RowLoA(),"  "_btmNum_" ")
	e  d
	. d nbaryDrawRowRaw(.nbArray,nbIndex,$$RowHiC()," "_topNum_" ")
	. d nbaryDrawRowRaw(.nbArray,nbIndex,$$RowLoA()," "_btmNum_" ")
	q nbIndex
	;
	;--------
	; DESCRIPTION: Rows in the order that we want to loop through them while making our key signature
	;--------
keySignatureRows() ;
	q $$RowLoF()_"^"_$$RowMdB()_"^"_$$RowHiE()_"^"_$$RowLoE()_"^"_$$RowLoA()_"^"_$$RowHiD()_"^"_$$RowHiG()_"^"_$$RowLoD()_"^"_$$RowLoG()_"^"_$$RowHiC()_"^"_$$RowHiF()
	;
	;--------
	; DESCRIPTION: Make a key signature noteblock
	; PARAMETERS:
	;  nbArray    (IN/OUT) - The noteblockArray structure.
	;  bits01to16 (IN)     - Bits 1-16 of key signature encoding. Bits 4-14 are relevant here.
	;  bits17to32 (IN)     - Bits 17-32 of key signature encoding. Bits 20-30 are relevant here.
	; RETURNS:     Index of the new noteblock in the noteblockArray.
	;--------
makeKeySignature(nbArray,bits01to16,bits17to32) ;
	n nbIndex,keySignatureRows,bit3,bitLo,bitHi,dir,ksRowsIndex,ksRowsLastIndex,rowIndex,col,charToDraw,rowTextToDraw
	;
	s nbIndex=$$nbaryAppend(.nbArray,5)
	d drawStaff(.nbArray,nbIndex,0)
	;
	s keySignatureRows=$$keySignatureRows()
	s ksRowsLastIndex=$L(keySignatureRows,"^")
	s bit3=$$bits(bits01to16,3,1)
	s dir=$s(bit3:-1,1:1)  ;direction - loop backwards or forwards through keySignatureRows
	s col=0
	f ksRowsIndex=1:1:ksRowsLastIndex d
	. s rowIndex=$p(keySignatureRows,"^",$s(dir>0:ksRowsIndex,1:(ksRowsLastIndex+1-ksRowsIndex)))
	. s bitLo=$$bits(bits01to16,rowIndex+1,1)
	. s bitHi=$$bits(bits17to32,rowIndex+1,1)
	. s charToDraw=$s(bitLo:$s(bitHi:"~",1:"b"),1:$s(bitHi:"#",1:$c(1)))
	. i charToDraw'=$c(1) d
	. . s:col=0 rowTextToDraw=charToDraw_$c(1,1,1,1)
	. . s:col=1 rowTextToDraw=$c(1)_charToDraw_$c(1,1,1)
	. . s:col=2 rowTextToDraw=$c(1,1)_charToDraw_$c(1,1)
	. . s:col=3 rowTextToDraw=$c(1,1,1)_charToDraw_$c(1)
	. . s:col=4 rowTextToDraw=$c(1,1,1,1)_charToDraw
	. . d nbaryDrawRow(.nbArray,nbIndex,rowIndex,rowTextToDraw)
	. . s col=(col+1)#5
	q nbIndex
	;
	;--------
	; DESCRIPTION: Width of each type of barline (starting with single barline of width 3)
	;--------
barlineNoteblockWidths() ;
	q "3^4^5^5^4^1"
	;
	;--------
	; DESCRIPTION: Make a barline noteblock
	; PARAMETERS:
	;  nbArray (IN/OUT) - The noteblockArray structure.
	;  byte    (IN)     - Bits 1-8 of barline encoding. Bits 5-7 are relevant here.
	; RETURNS:     Index of the new noteblock in the noteblockArray.
	;--------
makeBarline(nbArray,byte) ;
	n nbIndex,bits5to8,width,row
	;
	s bits5to8=$$bits(byte,5,4)
	s width=$p($$barlineNoteblockWidths(),"^",(bits5to8+1))
	s:width="" width=5  ;If bits5to8's piece does not exist then byte is invalid and we will write the 5-width string "ERROR"
	s nbIndex=$$nbaryAppend(.nbArray,width)
	d drawStaff(.nbArray,nbIndex,0)
	;
	f row=$$RowLoE():1:$$RowHiF() d
	. d drawBarlineRow(.nbArray,nbIndex,row,byte)
	q nbIndex
	;
	;--------
	; DESCRIPTION: Get the raw 80-char (16*5) text of a clef noteblock
	; PARAMETERS:
	;  clefType (IN) - 0 for treble clef, 1 for bass clef, 2 for percussion
	;--------
getClefText(clefType) ;
	i clefType=0 q "        _   / \--|-/  |/ --|-- /|  /-|_-|/| \|\|-|\_|_/--|--O_/                 "
	i clefType=1 q "               -__--/  \0O--|-   /0--/-- /   /----     -----                    "
	i clefType=2 q "               -----     ----- # # -#-#- # # -----     -----                    "
	q "ERRORE  O  R  R  R  E  O  R  R  R  E  O  R  R  R  E  O  R  R  R  E  O  R  RERROR"
	;
	;--------
	; DESCRIPTION: Make a clef noteblock
	; PARAMETERS:
	;  nbArray (IN/OUT) - The noteblockArray structure.
	;  byte    (IN)     - Bits 1-8 of clef encoding. Bits 7-8 are relevant here.
	; RETURNS:     Index of the new noteblock in the noteblockArray.
	;--------
makeClef(nbArray,byte) ;
	n nbIndex,bits7to8,clefText,row,rowText
	s nbIndex=$$nbaryAppend(.nbArray,5)
	s bits7to8=$$bits(byte,7,2)
	s clefText=$$getClefText(bits7to8)
	f row=0:1:$$RowMax() d
	. s rowText=$e(clefText,(5*($$RowMax()-row)+1),(5*($$RowMax()-row)+5))
	. d nbaryDrawRowRaw(.nbArray,nbIndex,row,rowText)
	q nbIndex
	;
	;
	;
	;
	;
	;*******************************************************
	; High-level functions dealing with multiple noteblocks
	;*******************************************************
	;
	;--------
	; ENUM ParseResult - the possible results of trying to parse one or more encoded bytes into a new noteblock
	;--------
ParseResultParsedNoteblock() q 0
ParseResultParsedAll() q 1
ParseResultUnexpectedTerminator() q 2
ParseResultInvalidByte() q 3
ParseResultIntervalError() q 4
	;
	;--------
	; DESCRIPTION: Parse one or more encoded bytes, creating a new noteblock
	; PARAMETERS:
	;  bytesArray (IN/OUT) - Array of encoded bytes being parsed.
	;  bytesIndex (IN/OUT) - Index in array of bytes. Calling this function usually increases it.
	;  nbArray    (IN/OUT) - The noteblockArray structure. This function will likely append a new noteblock.
	;  info       (IN/OUT) - Info stored between calls to this function.
	;                        info("beams") - number of beams (0-2) most recent note in this measure had, or 0 if no previous note in measure.
	;                        info("tied")  - whether most recent note (not necessarily in measure) had a tie on its right.
	;                        info("dyn")   - whether previous noteblock was dynamics text.
	; RETURNS:     One of the ParseResults
	;--------
parseBytes(bytesArray,bytesIndex,nbArray,info) ;
	n byte1,byte2,byte3,byte4  ;Rarely are all four bytes used, because most noteblocks are encoded in less than four
	n ret,noteblockType,nbIndex,bits1to2,bits3to4,bits5to6,bits01to16,bits17to32
	;
	;The noteblockType variable is used for maintaining the info parameter.
	;Might be set to "BARLINE", "DYNAMICS", or "NOTE" depending on the noteblock type.
	;Otherwise, default is "OTHER" which represents terminator, clef, time signature, or key signature.
	s noteblockType="OTHER"
	;
	;Create new noteblock (or modify current one in the case of dynamics text)
	s byte1=bytesArray(bytesIndex),bytesIndex=bytesIndex+1
	s bits1to2=$$bits(byte1,1,2)
	i bits1to2=0 d
	. s bits3to4=$$bits(byte1,3,2)
	. i bits3to4=0 d
	. . s bits5to6=$$bits(byte1,5,2)
	. . i bits5to6=0 d
	. . . i byte1=0 d  ;Terminator, 1 byte
	. . . . s nbIndex=""
	. . . . s ret=$$ParseResultParsedAll()
	. . . e  d
	. . . . s ret=$$ParseResultInvalidByte()
	. . e  i bits5to6=2 d  ;Clef, 1 byte
	. . . s %=$$makeClef(.nbArray,byte1)
	. . e  d
	. . . s ret=$$ParseResultInvalidByte()
	. e  i bits3to4=1 d  ;Barline, 1 byte
	. . s %=$$makeBarline(.nbArray,byte1)
	. . s noteblockType="BARLINE"
	. e  i bits3to4=2 d  ;Dynamics text, 3 bytes
	. . ;This is the only set of bytes that modifies the current noteblock rather than creating a new one.
	. . i bytesIndex<1 s ret=$$ParseResultInvalidByte() q  ;Can't have dynamics text without a preceding noteblock.
	. . i info("dyn") s ret=$$ParseResultInvalidByte() q   ;Can't have dynamics text twice consecutively.
	. . s byte2=bytesArray(bytesIndex),bytesIndex=bytesIndex+1 i byte2=0 s ret=$$ParseResultUnexpectedTerminator() q
	. . s byte3=bytesArray(bytesIndex),bytesIndex=bytesIndex+1 i byte3=0 s ret=$$ParseResultUnexpectedTerminator() q
	. . s nbIndex=$$nbaryCount(.nbArray)
	. . d drawDynamicsTextRow(.nbArray,nbIndex,byte1,byte2,byte3)
	. . s noteblockType="DYNAMICS"
	. e  d
	. . s ret=$$ParseResultInvalidByte()
	e  i bits1to2=1 d  ;Note, 2 bytes
	. s byte2=bytesArray(bytesIndex),bytesIndex=bytesIndex+1 i byte2=0 s ret=$$ParseResultUnexpectedTerminator() q
	. s %=$$makeNote(.nbArray,byte1,byte2,.info)
	. s noteblockType="NOTE"
	e  i bits1to2=2 d  ;Time signature, 1 byte
	. s %=$$makeTimeSignature(.nbArray,byte1)
	e  i bits1to2=3 d  ;Key signature, 4 bytes
	. s byte2=bytesArray(bytesIndex),bytesIndex=bytesIndex+1 i byte2=0 s ret=$$ParseResultUnexpectedTerminator() q
	. s byte3=bytesArray(bytesIndex),bytesIndex=bytesIndex+1 i byte3=0 s ret=$$ParseResultUnexpectedTerminator() q
	. s byte4=bytesArray(bytesIndex),bytesIndex=bytesIndex+1 i byte4=0 s ret=$$ParseResultUnexpectedTerminator() q
	. s bits01to16=(byte2*256+byte1),bits17to32=(byte4*256+byte3)
	. s %=$$makeKeySignature(.nbArray,bits01to16,bits17to32)
	;
	i ret'="" QUIT ret
	;
	;Update info parameter
	i noteblockType="BARLINE" s info("beams")=0
	e  i noteblockType="NOTE" s info("beams")=$$countNoteBeams(byte2)
	i noteblockType="NOTE" s info("tied")=$$noteheadTiedToNext(byte2)
	s info("dyn")=(noteblockType="DYNAMICS")
	;
	s ret=$$ParseResultParsedNoteblock()
	q ret
	;
	;--------
	; DESCRIPTION: Parse array of encoded bytes to create array of noteblocks
	; PARAMETERS:
	;  bytesArray (IN)  - Array of bytes (0b11111111-terminated) from which to read.
	;  nbArray    (OUT) - Array of noteblocks created from bytesArray.
	;  errorArray (OUT) - If an error occurs, this will be set to its index in bytesArray, otherwise to -1.
	; RETURNS:     One of the ParseResults
	;--------
parseBytesStartToEnd(bytesArray,nbArray,errorIndex) ;
	n bytesIndex,info,parseResult
	k nbArray,errorIndex  ;Kill output parameters
	;
	s bytesIndex=1
	s (info("beams"),info("tied"),info("dyn"))=0  ;Array of info maintained between calls to parseBytes
	;
	f  d  q:parseResult'=$$ParseResultParsedNoteblock()
	. s parseResult=$$parseBytes(.bytesArray,.bytesIndex,.nbArray,.info)
	s errorIndex=$s(parseResult=$$ParseResultParsedAll():-1,1:(bytesIndex-1))
	q parseResult
	;
	;
	;
	;
	;
	;**************************************************************************
	; Functions for converting a noteblockArray to a string
	;
	; The string contains one or more staves. Each staff contains 16 newline-terminated rows.
	; We handle a user-specified max staff/row width to ensure the string displays properly in the terminal.
	; If the width is large enough, there may be only one staff.
	;**************************************************************************
	;
	;--------
	; DESCRIPTION: Create the top row of the current staff.
	;              For performance, this subroutine is separate from appendStaffRowSubsequent, which handles
	;              the other 15 rows of the staff. This subroutine determines how many noteblocks fit in the
	;              staff based on the max width. It sets output parameter staffHeadIndexNext accordingly,
	;              and appendStaffRowSubsequent uses that information.
	; PARAMETERS:
	;  nbArray            (IN)     - The noteblockArray structure.
	;  staffHeadIndex     (IN)     - Index in first noteblock in current staff.
	;  staffHeadIndexNext    (OUT) - Will be set to index of first noteblock in next staff,
	;                                or to number of noteblocks + 1 if currently in last staff.
	;  row                (IN)     - Row number (0 to 15) in staff. Should be the top row, $$RowHiB().
	;  str                (IN/OUT) - Partially populated string, to which to append the new row text.
	;  maxStaffWidth      (IN)     - Max number of characters in the staff's string representation (not including newline).
	; NOTE:         Originally, appendStaffRowInitial and appendStaffRowSubsequent were not separated into
	;               two different subroutines. I noticed that they used a large portion of runtime, so I separated
	;               them to improve performance. They are similar in structure and function.
	;--------
appendStaffRowInitial(nbArray,staffHeadIndex,staffHeadIndexNext,row,str,maxStaffWidth) ;
	n strLengthLimit,nbIndex,nbCount,rowText,reachedNextStaff
	k staffHeadIndexNext
	s nbCount=$$nbaryCount(.nbArray)
	s strLengthLimit=$L(str)+maxStaffWidth
	f nbIndex=staffHeadIndex:1:nbCount d  q:reachedNextStaff
	. s rowText=$$nbaryGetRow(.nbArray,nbIndex,row)
	. i ($L(str)+$L(rowText))>strLengthLimit s reachedNextStaff=1 q
	. s str=str_rowText
	s str=str_$c(13,10)  ;Append newline
	s staffHeadIndexNext=$s(reachedNextStaff:nbIndex,1:nbCount+1)
	q
	;
	;--------
	; DESCRIPTION: Create another row of the current staff.
	;              Assumes that appendStaffRowInitial has already determined which noteblocks are in range
	;              for the current staff and that staffHeadIndex and staffHeadIndexNext are set accordingly.
	; PARAMETERS:
	;  nbArray            (IN)     - The noteblockArray structure.
	;  staffHeadIndex     (IN)     - Index in first noteblock in current staff.
	;  staffHeadIndexNext (IN)     - Index of first noteblock in next staff, or number of noteblocks + 1 if currently in last staff.
	;  row                (IN)     - Row number (0 to 15) in staff.
	;  str                (IN/OUT) - Partially populated string, to which to append the new row text.
	;--------
appendStaffRowSubsequent(nbArray,staffHeadIndex,staffHeadIndexNext,row,str) ;
	n nbIndex
	f nbIndex=staffHeadIndex:1:(staffHeadIndexNext-1) d
	. s str=str_$$nbaryGetRow(.nbArray,nbIndex,row)
	s str=str_$c(13,10)  ;Append newline
	q
	;
	;--------
	; DESCRIPTION: Convert a noteblockArray to a single string.
	; PARAMETERS:
	;  nbArray       (IN) - The noteblockArray structure.
	;  maxStaffWidth (IN) - Max width of a staff in characters. Should be no less than $$NoteblockMaxWidth().
	; RETURNS:     The result of converting the noteblocks to a single string
	; NOTE:        I tried preallocating str as in music.c (setting $e(str,estimatedLength+1)="" before looping
	;              and using $e instead of concatenation to add text).
	;              It didn't make a significant impact on performance, so I changed it to normal concatenation.
	;              It seems that the language already has a good string implementation.
	;--------
noteblocksToString(nbArray,maxStaffWidth) ;
	n staffHeadIndex,staffHeadIndexNext,staffHeadIndexLimit,row,str
	i maxStaffWidth<$$NoteblockMaxWidth() QUIT ""
	;Loop over staves until last noteblock processed
	s staffHeadIndex=1,staffHeadIndexLimit=$$nbaryCount(.nbArray)
	f  d  q:staffHeadIndex>staffHeadIndexLimit
	. ;Loop over rows in staff. Rows are numbered from bottom, but we're printing from top, so loop backwards.
	. s row=$$RowMax()
	. d appendStaffRowInitial(.nbArray,staffHeadIndex,.staffHeadIndexNext,row,.str,maxStaffWidth)
	. f row=(row-1):-1:0 d
	. . d appendStaffRowSubsequent(.nbArray,staffHeadIndex,staffHeadIndexNext,row,.str)
	. s str=str_$c(13,10)  ;Append another newline to separate staves
	. s staffHeadIndex=staffHeadIndexNext
	q str
	;
	;
	;
	;
	;
	;*********
	; Main IO
	;*********
	;
	;--------
	; DESCRIPTION: Size in bytes of largest file we would try to read from.
	;--------
FileSizeMax() ;
	q 99999
	;--------
	; DESCRIPTION: Attempts to open file, decode it, and print music.
	; PARAMETERS:
	;  filepath (IN) - User-entered file path and name.
	;  width    (IN) - User-entered string for max staff width (invalid if less than 5).
	;                  Null means they didn't enter one, in which case we don't enforce a max width.
	;--------
tryReadFile(filepath,width) ;
	n bad,widthToUse,bytesStr,byteIndex,bytesArray,nbArray,parseResult,errorIndex,str
	;
	;Validate filepath not null
	i filepath="" d  QUIT  ;If filepath is null, `OPEN filepath` will throw an error, not just set $TEST=0.
	. w "  File not specified",!
	;
	;Validate widthStr and save to widthToUse
	i width'="" d
	. i width'=(width\1) d  s bad=1 q
	. . w "  Invalid width",!
	. i width<$$NoteblockMaxWidth() d  s bad=1 q
	. . w "  Invalid width "_width_" < "_$$NoteblockMaxWidth(),!
	. s widthToUse=width
	e  d
	. s widthToUse=99999  ;Will effectively be ignored
	QUIT:bad
	;
	;Open file
	OPEN filepath:("R"):1  ;Open file in Read mode with 1-second timeout
	s bad='$TEST  ;This would not work without the timeout, because including the timeout allows OPEN to set $TEST
	i bad d  QUIT
	. w "  Unable to open file "_filepath,!
	;
	;Read and close file
	USE filepath
	READ bytesStr#($$FileSizeMax()+1):1  ;Read from file to bytesStr with 1-second timeout
	s bad='$TEST  ;This would not work without the timeout, because including the timeout allows READ to set $TEST
	CLOSE filepath
	i bad d  QUIT
	. w "  Unable to read from file "_filepath,!
	;
	;Validate file size
	i bytesStr="" d  QUIT
	. w "  File is empty: "_filepath,!
	e  i $L(bytesStr)>$$FileSizeMax() d  QUIT
	. w "  File is too long (>"_$$FileSizeMax()_" bytes): "_filepath
	;
	;String of bytes to array of bytes
	s bytesArray(0)=$L(bytesStr)
	f byteIndex=1:1:$L(bytesStr) d
	. s bytesArray(byteIndex)=$a($e(bytesStr,byteIndex))
	;
	;Array of bytes to list of noteblocks
	s parseResult=$$parseBytesStartToEnd(.bytesArray,.nbArray,.errorIndex)
	i parseResult'=$$ParseResultParsedAll() d  QUIT
	. i parseResult=$$ParseResultInvalidByte() d
	. . w "  Invalid value "_bytesArray(errorIndex)_" at byte "_errorIndex,!
	. e  i parseResult=$$ParseResultUnexpectedTerminator() d
	. . w "  Invalid terminator (0b00000000) at byte "_errorIndex,!
	. e  d
	. . w "  Internal error while parsing noteblocks",!
	;
	;List of noteblocks to string
	s str=$$noteblocksToString(.nbArray,widthToUse)
	i str="" d  QUIT
	. w "  Internal error while converting noteblocks to string",!
	w str
	q
	;
	;--------
	; DESCRIPTION: Returns example string printed when user chooses option V.
	;              If an error occurs, writes error information and returns null.
	;--------
strExample() ;
	n bytesArray,parseResult,nbArray,nbArrayCount,errorIndex,str,msg
	d genExampleBytes(.bytesArray)
	s parseResult=$$parseBytesStartToEnd(.bytesArray,.nbArray,.errorIndex)
	i parseResult'=$$ParseResultParsedAll() d  QUIT ""
	. s nbArrayCount=$$nbaryCount(.nbArray)
	. s msg="  Internal error: parse result "_parseResult_", error index "_errorIndex
	. s msg=msg_", "_$s(nbArrayCount>0:"a",1:"no")_" noteblock exists"
	. w msg,!
	s str=$$noteblocksToString(.nbArray,$$ExampleWidth())
	q str
	;
	;--------
	; DESCRIPTION: Subtract two moments in time
	; PARAMETERS:
	;  day1 (IN) - Days counter in later moment
	;  day0 (IN) - Days counter in earlier moment
	;  sec1 (IN) - Seconds counter in later moment (up to 86400 seconds in a day)
	;  sec0 (IN) - Seconds counter in earlier moment (up to 86400)
	; RETURNS: The number of seconds from the earlier moment to the later moment
	;--------
subtractTime(day1,day0,sec1,sec0) ;
	q (86400*(day1-day0))+(sec1-sec0)  ;86400 seconds in a day
	;
	;--------
	; DESCRIPTION: Test performance by constructing ($$)strExample count times
	; PARAMETERS:
	;  count (IN) - How many times to call ($$)strExample. Should be >= 10.
	;--------
testPerformance(count) ;
	n tenthOfCount,tenthsDone,ii,done,time0,time1,timeDiff
	;
	;Validate count argument
	i count'=(count\1) d  QUIT
	. w "  Invalid count",!
	i count<10 d  QUIT
	. w "  Invalid count "_count_" < 10",!
	;
	;The following is over-optimized for the speed of the loop.
	;In particular, the loop does direct comparison to 0 with no modulus involved.
	;Also, most of the time it doesn't execute the Do command to enter another dot block.
	s tenthOfCount=count\10
	s tenthsDone=0  ;How many times we have looped count\10 times
	s ii=tenthOfCount+(count#10)  ;ii will count down to 0 ten times
	w "  Done: 00%"  ;No newline, later we'll write more to this line
	s time0=$h
	f  s %=$$strExample(),ii=ii-1 d:ii'>0  q:done
	. ;Handle ii=0 - countdown again or end loop
	. s tenthsDone=tenthsDone+1
	. w " "_tenthsDone_"0%"  ;Still no newline
	. s ii=tenthOfCount
	. s done=(tenthsDone>=10)
	s time1=$h
	w !  ;Newline
	;
	s timeDiff=$$subtractTime($p(time1,","),$p(time0,","),$p(time1,",",2),$p(time0,",",2))
	w "  Example output constructed "_count_" times in <"_timeDiff_" seconds",!
	q
	;
	;--------
	; DESCRIPTION: Main entry point that a user can call from the command line
	; PARAMETERS:
	;  mode (IN) - One of the following characters:
	;              H (show help text), E (show encoding text) V (show example of visual output),
	;              F (read from file), P (performance test.
	;  arg1 (IN) - In mode F, this is the file path.
	;              In mode P, this is the count of times to rerun the program.
	;              In other modes, this should be null.
	;  arg2 (IN) - In mode F, this is the maximum page width (should be from 5 to 255).
	;              In other modes, this should be null.
	;--------
main(mode,arg2,arg3) ;
	i mode="E",arg2="",arg3="" d
	. w $$strEncoding()
	e  i mode="V",arg2="",arg3="" d
	. w $$strExample()
	e  i mode="F" d
	. d tryReadFile(arg2,arg3)
	e  i mode="P",arg2'="",arg3="" d
	. d testPerformance(arg2)
	e  d  ;This also handles mode H
	. w $$strHelp()
	q
	;
	q  ;End of routine
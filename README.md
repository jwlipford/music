# music
 music.exe prints sheet music (drawn with ASCII characters) from an encoded file.

I was inspired by three desires:

* To see how much information density I could squeeze out of a few bytes
  * I never needed more than four bytes per 5-by-16-character block of text. I was able to squeeze the encoding of one note into two bytes.
  * The example text below, 1534 characters long (not including spaces preceding newlines), is encoded in 87 bytes - a ratio of about 18:1.
* To further familiarize myself with C
  * I learned the great benefit of prefixing pointer variable names with 'p'!
* To translate the C program into C# and compare performance
  * I did so, but not as closely as possible. For example, I translated one C struct to a C# object with its own methods. I thought I would have done it this way if I had implemented in C# first.
  * It turns out that the C version runs a lot faster. On my machine, `music.exe -p 543210` runs in 4 seconds, and `music_cs.exe -p 543210` runs in 13 seconds.

#### Other versions

* I wrote an F# version. It was quite fun. However, that version is not at all optimized and runs about a hundred times slower than the C# version. On my machine, `music_fs.exe -p 5432` runs in 33 seconds.
* I got most of the way through a Fortran version before I lost interest. That version's performance compares to the C version's. However, given the choice, I would much rather code in C; I see the reason for C's higher popularity.
* I wrote a MUMPS version. It was fun. Because MUMPS is such a different language from the others, there are interesting differences in the data structures. What in C and C# was a linked list of two-dimensional arrays is in MUMPS a three-level tree of strings.

#### Example of visual style

```
$>music.exe -v

    _
   / \  #
---|-/#--------------------------------------+--------------------------------+------
   |/                  ______________        |          ______________        |
---|-----#------------|----|----|----|-------|---------|----|----|----|-------|------
  /|   #    4    O    |    |    |    |       |    O    |    |    |    |       |    O
-/-|_-----------/-----|----|----|----|-------|---/-----|----|----|----|-------|---/--
 |/| \      4         |  (@|.   |  (@|       |         |  (@|.   |  (@|       |
-|\|-|--------------(@|-------(@|.--------|--|-------(@|-------(@|.--------|--|------
 \_|_/                                    |  |                             |  |
---|------------------------------------(@|--+---------------------------(@|--+------
 O_/


                    mp




--------------------------+-------------------------------------------++------
    ______________        |          ______________                   ||
---|----|----|----|-------|---------|----|----|----|--------.----.----||------
   |    |    |    |       |    O    |    |    |    |       |@)  |@)_ 0|| _|_)_
---|----|----|----|-------|---/-----|----|----|----|-------|----|-----||--|---
   |  (@|.   |  (@|       |         |  (@|.   |  (@|       |    |    0||  |
-(@|-------(@|.--------|--|-------(@|-------(@|.--------|\-|____|-----||------
                       |  |                             |  |____|     ||
---------------------(@|--+---------------------------(@|-------------++------



                                   <<<<<<<<<<<<<<<<<<<<<<<< f


$>
```
#### File Creation and Encoding

###### File Creation

One way to create an encoded file is to use the PowerShell `set-content` cmdlet. After that, run the program with the file as its argument. For example:
```
$b=[byte[]]@(0x53,0x61,0x75,0x63,0x65)
set-content encoded_song $b -encoding byte
.\music.exe encoded_song
```

###### File Encoding

* The file consists of groups of bytes that represent "noteblocks", which are rectangles of text 16
  characters high and at most 5 characters wide. At most 4 bytes represent one noteblock.
* In the following descriptions, we count bits from the right, the 1's place, starting at 1.
  For example, in the bit string 10010, bits 2 and 5 are the 1s; bits 1, 3, and 4 are the 0s.
  For two bytes, their bits numbered in base 32 are: 87654321 GFEDCBA9.
* Terminator (1 byte):
  * Bits 1-8: Always 00000000
* Note (2 bytes):
  * Bits 1-2:   Always 01
  * Bits 3-6:   Rest (0) or pitches low B (1) to middle B (8) to high B (15)
  * Bits 7-8:   Accidentals - none (0), flat ('b') (1), natural ('~') (2), or sharp ('#') (3)
  * Bits 9-12:
    * Appearance - Invalid (0), unused (1), Breve (2), whole (3), half (4), quarter (5), eighth (6-14 even),
      sixteenth (7-15 odd).
    * For eighths and sixteenths, there are five encodings each that indicate whether
      the note is flagged or beamed and, if beamed, the beam height:
      * Flagged (6-7) VS beamed (8-15).
      * Beamed on left only with number of left-side beams determined by previous note (12-15), VS
        beamed on right at least once with number of left-side beams determined by previous note (8-11).
      * Beam two spaces away (8-9, 12-13) VS three spaces away (10-11, 14-15).
  * Bit  13:    Dotted
  * Bit  14:    Tie/slur after
  * Bits 15-16: Articulations - None (0), staccato (1), accent (2), tenuto (3)
* Time change (1 byte):
  * Bits 1-2: Always 10
  * Bits 3-4: Bottom number - 1, 2, 4, or 8 (encoded as 0 to 3)
  * Bits 5-8: Top number - 1 to 16 (encoded as 0 to 15)
* Key change (4 bytes):
  * Bits 1-2: Always 11
  * Bit  3:   Arrangement of accidentals - Resembling Db major scale (0) or B major scale (1)
  * Bits 4-14, 20-30:
    * Pitches go from low D (bits 4 and 20) to high G (bits 14 and 30).
    * If a pitch has a 1 in the first bit string but not the second, it is flat ('b').
    * If a pitch has a 1 in the second bit string but not the first, it is sharp ('#').
    * If a pitch has a 1 in both bit strings, it is natural ('~').
  * Bits 15-16, 17-19, 31-32:
    * Use these bits to make sure each byte has at least one 1 so it's not a terminator.
* Barline (1 byte):
  * Bits 1-4: Always 0100
  * Bit  5-7: Type of barline - single (0), double (1), left repeat (2), right repeat (3), both repeats (4), blank column (5)
  * Bit  8:   Unused
* Dynamics text (3 bytes):
  * Applies to the previous noteblock. (Non-note noteblocks might need to have part of a crescendo/
    decrescendo under them.) Invalid if this is the first byte.
  * Bits 1-4: Always 1000
  * Bits 5-8, 9-12, 13-16, 17-20, 21-24:
    * Each group of four bits represents one of these characters (encoded as 1-13, 0 invalid, 14-15 unused):
      * Null, space, '<', '>', '.', 'c', 'd', 'e', 'f', 'm', 'p', 'r', 's'
    * These 12 characters can make text such as " ppp ", "cresc", " decr", " mp<<", "<<f>>", etc.
* Clef (1 byte):
  * Elsewhere in these descriptions, pitch names assume treble clef, but here you can draw a different clef.
  * Bits 1-6: Always 100000
  * Bits 7-8: Type - Treble (0), bass (1), percussion (2)
* If you ever see a capital 'E' or the string "ERROR" in the generated music, your file has invalid input.

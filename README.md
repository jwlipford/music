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


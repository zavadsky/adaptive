The program implements algorithm from the paper "Fast practical adaptive encoding on large alphabets" by I.O. Zavadskyi, submitted to DCC'25.
It can be compiled with
g++.exe -Wall -fexceptions -w -std=gnu++17  -c
oprions for Windows. To run it on Linux the time measurement functions in the main.cpp shoul be changed (#define measure_start(), #define measure_end()).

The program has 2 command line arguments: name of the text file and number of iterations to test decoding and decoding time. E.g.,
adaptive.exe text.txt 10

Only alphanumeric stream is encoded on a word-based alphabet, i.e. punctuation signs are eliminated and in what remains a sequence of characters between two consequtive spaces is considered a word, i.e. an alphabet symbol.

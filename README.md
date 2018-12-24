### File Duplicator


This program duplicates any source.txt into destination.txt using asynchronous read and write methods in [aio](http://man7.org/linux/man-pages/man7/aio.7.html) library.

---

#### Usage: 

Compile with:

`gcc main.c -pthread -lrt -o main`


If you get any warnings you can add `-w`


`./main <source_path> <destination_path> <number_of_threads>`



# fibonacci-heaps_c
Implementation of the Fibonacci Heap priority queue, ready for user applications programming. Written in C, requires the GNU C Library.

They are a very efficient kind of priority queue, but you'll have to look elsewhere for a complete description of what such a data structure can do, and why this one specifically is so fast. Currently, keys are 64 bits unsigned integers and elements are _void *_, so anything that fits in 8 bytes will do. It relies heavily on dynamic memory and the heap, and supports dynamic data as well (elements could be pointers to the heap themselves). More implementations supporting different data types for keys might be added in the future.

**WARNING:** Requires the [Double Linked Lists](https://github.com/robmasocco/double-linked-lists_c) library to work. See the header file for a more detailed description.

## Can I use this?

If you stumbled upon here and find this suitable for your project, or think this might save you some work, sure!
Just credit the use of this code, and although I used this stuff in projects and tested it, there's no 100% guarantee that it's bug-free. Feel free to open an issue if you see something wrong.

Also, this code is protected by the MIT license as per the attached *LICENSE* file.
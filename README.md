tinyhttp
========
Tiny (as in minimal) implementation of an HTTP response parser. The parser
itself is only dependent on:
* `<ctype.h> - tolower`
* `<string.h> - memcpy`

Contact
-------
[@MatthewEndsley](https://twitter.com/#!/MatthewEndsley)  
<https://github.com/mendsley/tinyhttp>

License
-------
Copyright 2012 Matthew Endsley

This project is governed by the BSD 2-clause license. For details see the file
titled LICENSE in the project root folder.

Compiling
---------
`gcc -c *.c && g++ -std=c++0x example.cpp *.o -o example`

`./example` will fetch the root of <http://nothings.org>

# C API for JSON (CAJ): an event-based parser for JSON in C language

Typically, JSON is parsed by a tree-based parser unlike XML that can be parsed
by a tree-based parser or an event-based parser. Event-based parsers are fast
and have a low memory footprint, but a drawback is that it is cumbersome to
write the required event handlers. Tree-based parsers make the code easier to
write, to understand and to maintain but have a large memory footprint as a
drawback. Sometimes, JSON is used for huge files such as database dumps that
would be preferably parsed by event-based parsing, or so it would appear at a
glance, because a tree-based parser cannot hold the whole parse tree in memory
at the same time, if the file is huge.

## Use cases

The main use case for CAJ is handling huge database dumps stored in JSON
format. Usually JSON is not used for huge database dumps and XML is used
instead because event-based parsers are more readily available for XML than
they are for JSON. However, XML is ill-suited as a data serialization format
and contains numerous bad features such as duality of attributes and elements.
Using JSON might be more optimal, and the only obstacle to using JSON is the
lack of event-based parsers. CAJ aims to change the situation.

Also CAJ allows unserializing JSON files into custom data structures. In
particular, any JSON library needs to implement an associative array, a data
type that is not available in C language. Thus any C JSON library will probably
have a custom implementation of the associative array, which may differ from
the implementation that you want to use in your program.

## How to build

CAJ is built using Stirmake. How to install it:

```
git clone https://github.com/Aalto5G/stirmake
cd stirmake
git submodule init
git submodule update
cd stirc
make
./install.sh
```

This installs stirmake to `~/.local`. If you want to install to `/usr/local`,
run `./install.sh` by typing `sudo ./install.sh /usr/local` (and you may want
to run `sudo mandb` also).

If the installation told `~/.local` is missing, create it with `mkdir` and try
again. If the installation needed to create `~/.local/bin`, you may need to
re-login for the programs to appear in your `PATH`.

Then build CAJ by:

```
cd caj
smka
```

## License

All of the material related to CAJ is licensed under the following MIT
license:

Copyright (C) 2020 Juha-Matti Tilli

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

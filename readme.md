# gmip

A gemtext to slideshow generator and viewer.

[![asciicast](https://asciinema.org/a/577542.svg)](https://asciinema.org/a/577542)


## Description

gmip generates slideshows from gemtext files. It is heavily inspired by mdp[^1].

[^1]: https://github.com/visit1985/mdp

Additionally to gemtext syntax[^2], gmip also understands, at the start of a
line:
- `---` to delimit slides,
- `^` to stop output (it enables progressive revealing),
- `%title:My title`, `%author:Me` and `%date:1970-01-01` ASCII metadata.

[^2]: https://gemini.circumlunar.space/docs/gemtext.gmi


## Usage

To build and install, edit `config.mk` to match your local setup and run
`make install` (if necessary as root). While `cc` is the default compiler,
`tcc` is strongly advised.

To run gmip, just run `gmip path/to/file`.

A `demo.gmi` is provided for quick testing and syntax cheatsheet.


## Feedback/contact information

Your feedback is greatly appreciated! If you have any comment on gmip,
whether it's on the user side or the code, send an email at arthur@jaquin.xyz.

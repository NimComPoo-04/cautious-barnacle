# cautious-barnacle

Simple 'the one who shall not be named' clone.

## How to play

* <kbd>&uarr;</kbd> Rotates piece by 90 degrees
* <kbd>&darr;</kbd> Increase Desend speed of piece
* <kbd>&larr;</kbd> Moves piece to the left
* <kdb>&rarr;</kdb> Moves piece to the right
* <kbd>&nbsp;</kbd> Pauses the game
* <kbd>Esc</kbd>    Exits the game and also displays the score

## Building and Running Steps

For Linux
```bash
gcc -Wall -Wextra -ggdb bothoristis.c -lX11
./a.out
```

For Windows (mingw64)
```bat
gcc -Wall -Wextra -ggdb bothoristis.c -luser32 -lgdi32 -o game.exe
game
```

## Screenshot

![gamplay picture](https://github.com/NimComPoo-04/cautious-barnacle/blob/main/scrnsht.png?raw=true)

## Minor Hiccup

The C runtime library on c or MSVCRT.dll may be missing on your computer, this will result in an error.
If that happens you can install the Visual Studio C++ runtime and the program should work fine.
If any problems happen don't hesitate to open an issue.

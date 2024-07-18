For the compilation from linux to:

    -linux avec débogage: 
        gcc -g -Wall -Wextra -pedantic -fsanitize=address -o bullet bullet.c -lm -lSDL2 -lSDL2_ttf
    -linux: 
        gcc -o bullet bullet.c -lm -lSDL2 -lSDL2_ttf
    -windows: 
        x86_64-w64-mingw32-gcc -o bullet.exe bullet.c -lm -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -L/path/to/SDL2/lib -I/path/to/SDL2/include
    -mac: 
        o64-clang -o bullet bullet.c -lm -lSDL2 -lSDL2_ttf -L/path/to/SDL2/lib -I/path/to/SDL2/include


For the compilation from linux to:

    -linux avec débogage: 
        gcc -g -Wall -fsanitize=address -o bullet bullet.c -lm -lSDL2
    -linux: 
        gcc -o bullet bullet.c -lm -lSDL2
    -windows: 
        x86_64-w64-mingw32-gcc -o bullet.exe bullet.c -lm -lmingw32 -lSDL2main -lSDL2 -L/path/to/SDL2/lib -I/path/to/SDL2/include
    -mac: 
        o64-clang -o bullet bullet.c -lm -lSDL2 -L/path/to/SDL2/lib -I/path/to/SDL2/include


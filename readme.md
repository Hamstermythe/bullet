For the compilation from linux to:

    -linux avec débogage: 
        gcc -g -Wall -fsanitize=address -o spatial_ship spatial_ship.c -lm -lSDL2
    -linux: 
        gcc -o my_program my_program.c -lm -lSDL2
    -windows: 
        x86_64-w64-mingw32-gcc -o my_program.exe my_program.c -lm -lmingw32 -lSDL2main -lSDL2 -L/path/to/SDL2/lib -I/path/to/SDL2/include
    -mac: 
        o64-clang -o my_program my_program.c -lm -lSDL2 -L/path/to/SDL2/lib -I/path/to/SDL2/include


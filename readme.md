This program is a simple low fonctionnal game dedicated to code demonstration.


If you want just test the game please go in `your_os-bin` and run binary file.
You play a spatial-ship who need shot the obstacle for up him score.
-Key arrow-up give speed boost.
-Key arrow-right rotate on right.
-Key arrow-left rotate on left.
-Key space shot a bullet.
You need touch ten times an obstacle for destruct him and win one point.




Devs-informations:

    If SDL2 is correctly installed on your device, you can use this command for the compilation from linux to:
        -linux avec débogage: 
            gcc -g -Wall -Wextra -pedantic -fsanitize=address -o bullet bullet.c -lm -lSDL2 -lSDL2_ttf
        -linux: 
            gcc -o bullet bullet.c -lm -lSDL2 -lSDL2_ttf
        -windows: 
            x86_64-w64-mingw32-gcc -o bullet.exe bullet.c -lm -lmingw32 -lSDL2main -lSDL2 -lSDL2_ttf -L/path/to/include/SDL2/lib -I/path/to/SDL2/include
        -mac (not tested): 
            o64-clang -o bullet bullet.c -lm -lSDL2 -lSDL2_ttf -L/path/to/SDL2/lib -I/path/to/SDL2/include


    If you encounter problem with compiler you can use personal directory architecture for simplify the command and file access, example:
        -------------------------------------------
        directory and file organization \/
            ________________________________
            >win-SDL2/
                >SDL2-windows/
                    >include/
                        >SDL2/
                            SDL.h
                            SDL_ttf.h
                    >lib/
                        >x64/
                            libSDL2.a
                            libSDL2.dll.a
                            SDL2.dll
                            libSDL2_ttf.a
                            libSDL2_ttf.dll.a
                            SDL2_ttf.dll
            ________________________________
            x86_64-w64-mingw32-gcc -v -o bullet.exe bullet.c -I"/path/to/win-SDL2/SDL2-windows/include" -L"/path/to/win-SDL2/SDL2-windows/lib/x64" -lSDL2 -lSDL2_ttf







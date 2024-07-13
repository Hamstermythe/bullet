#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>

// Structure Definitions
typedef struct {
    int x;
    int y;
} Point;

typedef struct {
    float x;
    float y;
} Angle;

typedef struct {
    int last_bullet_shot;
    int score;
    int speed;
    int health;
    int collisioning;
    Point size;
    Point position;
    Angle angle;
} SpatialShip;

// Déclaration de la structure Bullet
typedef struct Bullet Bullet;
struct Bullet {
    Point position;
    Angle angle;
    int distance;
    int speed;
    int radius;
    int damage;
    int collisioning;
};

typedef struct {
    Point position;
    int radius;
    int health;
} Asteroid;

// Constants
#define SPATIAL_SHIP_SPEED 5
#define ASTEROID_VIEWING_DISTANCE 360
#define BULLET_COOLDOWN_MS 250
#define BULLET_LIFETIME 1200
#define VISIBLE_BLOCK_NUMBER 9
#define ASTEROID_PER_BLOC 4

// Globals
Point game_surface = {1000000, 1000000};
Point wnd_size = {640, 480};
Point spatial_ship_screen_position = {320, 360};

// Function Declarations
Point NewAsteroidPosition();
Asteroid* ChunkOfSpace(int bloc_position_y, int bloc_position_x);
Asteroid** lineOfSpace(int bloc_position_y);
Asteroid*** Space();
void FreeSpace(Asteroid*** space);

Point gameSurfaceAntiDebordement(Point position);

Bullet shoot(SpatialShip ship);
Bullet moveBullet(Bullet bullet);
Bullet* AddBullet(Bullet* bullets, int *bullet_size, SpatialShip spatial_ship);
Bullet* RemoveBullet(Bullet* bullets, int *bullet_size);

// Main Function
//int main(int argc, char* argv[]) {
int main() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window* window = SDL_CreateWindow("Space Shooter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, wnd_size.x, wnd_size.y, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        fprintf(stderr, "Could not create renderer: %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SpatialShip ship = {0, 0, SPATIAL_SHIP_SPEED, 100, 0, {20, 20}, {320, 360}, {0, -1}};
    Bullet* bullets = NULL;
    int bullet_size = 0;
    Asteroid*** asteroids = Space();
    printf("Space created\n");
    Uint32 lastBulletShotTime = SDL_GetTicks();

    bool running = true;
    SDL_Event event;
    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_ESCAPE:
                        running = false;
                        break;
                    case SDLK_LEFT:
                        //rotateSpatialShipLeft(&ship);
                        break;
                    case SDLK_RIGHT:
                        //rotateSpatialShipRight(&ship);
                        break;
                    case SDLK_SPACE:
                        if (SDL_GetTicks() - lastBulletShotTime > BULLET_COOLDOWN_MS) {
                            Bullet* temp = AddBullet(bullets, &bullet_size, ship);
                            if (temp != NULL) {
                                bullets = realloc(temp, bullet_size * sizeof(Bullet));
                            } else {
                                bullets = NULL;
                            }
                            //bullet_size++;
                            //bullets = AddBullet(bullets, bullet_size, ship);
                            lastBulletShotTime = SDL_GetTicks();
                        }
                        break;
                }
            }
        }

        printf("before move\n");
        //Move(&bullets, bullet_size, &ship);
        for (int i = 0; i < bullet_size; i++) {
            printf("bullet %d move\n", i);
            bullets[i] = moveBullet(bullets[i]);
            //bullets[i].move(&bullets[i]);
        }
        printf("before collision\n");
        //Collision(&bullets, bullet_size, asteroids, &ship);
        printf("Bullet size before: %d\n", bullet_size);
        Bullet* temp = RemoveBullet(bullets, &bullet_size);
        if (temp != NULL) {
            bullets = temp;//realloc(temp, bullet_size * sizeof(Bullet));
            //free(temp);
        } else {
            bullets = NULL;
        }
        // bullets = RemoveBullet(&bullets, &bullet_size);
        printf("Bullet size after: %d\n", bullet_size);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        //Scene(renderer, asteroids, bullets, bullet_size, ship);
        SDL_RenderPresent(renderer);

        SDL_Delay(16);  // Roughly 60 frames per second
    }

    FreeSpace(asteroids);
    if (bullets != NULL) {
        free(bullets);
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}


// Implementations of previously declared functions
Point NewAsteroidPosition() {
    return (Point) {rand() % wnd_size.x, rand() % wnd_size.y};
}

Asteroid* ChunkOfSpace(int bloc_position_y, int bloc_position_x) {
    // int asteroid_in_this_bloc = 4;
    Asteroid* asteroids = malloc(ASTEROID_PER_BLOC * sizeof(Asteroid));
    if (asteroids == NULL) {
        fprintf(stderr, "Allocation mémoire pour ChunkOfSpace a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < ASTEROID_PER_BLOC; i++) {
        Point position = NewAsteroidPosition();
        Point real_position = {position.x + (bloc_position_x * wnd_size.x), position.y + (bloc_position_y * wnd_size.y)};
        asteroids[i].position = real_position;
        asteroids[i].radius = wnd_size.x / 5;
        asteroids[i].health = 100;
    }
    return asteroids;
}

Asteroid** lineOfSpace(int bloc_position_y) {
    int bloc_number = (game_surface.x / wnd_size.x) + 1;
    Asteroid** line = malloc(bloc_number * sizeof(Asteroid*));
    if (line == NULL) {
        fprintf(stderr, "Allocation mémoire pour lineOfSpace a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < bloc_number; i++) {
        line[i] = ChunkOfSpace(bloc_position_y, i);
    }
    return line;
}

Asteroid*** Space() {
    int lign_number = (game_surface.y / wnd_size.y) + 1;
    Asteroid*** space = malloc(lign_number * sizeof(Asteroid**));
    if (space == NULL) {
        fprintf(stderr, "Allocation mémoire pour Space a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < lign_number; i++) {
        space[i] = lineOfSpace(i);
        int asteroid_number = game_surface.x / wnd_size.x * i * ASTEROID_PER_BLOC;
        printf("lign %d maked, number of asteroid is %d\n", i, asteroid_number);
    }
    return space;
}

void FreeSpace(Asteroid*** space) {
    int lign_number = game_surface.y / wnd_size.y;
    int bloc_number = game_surface.x / wnd_size.x;
    for (int i = 0; i < lign_number; i++) {
        for (int j = 0; j < bloc_number; j++) {
            free(space[i][j]);
        }
        free(space[i]);
    }
    free(space);
}





Point gameSurfaceAntiDebordement(Point position) {
    if (position.x < 0) {
        position.x = position.x + game_surface.x;
    } else if (position.x > game_surface.x) {
        position.x = position.x - game_surface.x;
    }
    if (position.y < 0) {
        position.y = position.y + game_surface.y;
    } else if (position.y > game_surface.y) {
        position.y = position.y - game_surface.y;
    }
    return position;
}

Bullet shoot(SpatialShip ship) {
    Bullet bullet = {ship.position, ship.angle, 0, 20, 10, 10, 0};
    return bullet;
}

Bullet moveBullet(Bullet bullet) {
    bullet.position.x += (int)(bullet.angle.x * (float)(bullet.speed));
    bullet.position.y += (int)(bullet.angle.y * (float)(bullet.speed));
    bullet.distance += bullet.speed;
    bullet.position = gameSurfaceAntiDebordement(bullet.position);
    return bullet;
}

Bullet* AddBullet(Bullet* bullets, int *bullet_size, SpatialShip spatial_ship) {
    (*bullet_size)++;
    Bullet bullet = shoot(spatial_ship);
    Bullet* new_bullets = malloc((*bullet_size) * sizeof(Bullet));
    // bullets = realloc(bullets, (*bullet_size) * sizeof(Bullet));
    if (new_bullets == NULL) {
        fprintf(stderr, "Reallocation de mémoire pour les balles a échoué.\n");
        //return;
        return NULL;
    }
    for (int i = 0; i < (*bullet_size) - 1; i++) {
        new_bullets[i] = bullets[i];
    }
    new_bullets[(*bullet_size) - 1] = bullet;
    //(*bullets)[(*bullet_size) - 1] = bullet;
    return new_bullets;
}

Bullet* RemoveBullet(Bullet* bullets, int *bullet_size) {
    int new_bullet_size = 0;
    for (int i = 0; i < *bullet_size; i++) {
        if (bullets[i].distance < BULLET_LIFETIME) {
            new_bullet_size++;
        }
    }
    if (new_bullet_size == 0) {
        //free(*bullets);
        *bullet_size = 0;
        return NULL;
    }
    Bullet* new_bullets = malloc(new_bullet_size * sizeof(Bullet));
    if (new_bullets == NULL) {
        fprintf(stderr, "Reallocation de mémoire pour les balles a échoué.\n");
        //return;
        return NULL;
    }
    int n = 0;
    for (int i = 0; i < *bullet_size; i++) {
        printf("   index: %d, distance: %d, vitesse: %d\n", i, bullets[i].distance, bullets[i].speed);
        if (bullets[i].distance < BULLET_LIFETIME) {
            new_bullets[n] = bullets[i];
            n++;
        }
    }
    // (*bullets) = new_bullets;
    //*bullets = realloc(new_bullets, (*bullet_size) * sizeof(Bullet));
    (*bullet_size) = new_bullet_size;
    return new_bullets;
}

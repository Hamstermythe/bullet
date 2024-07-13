#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>  // Pour les fonctions mathématiques (pow, sqrt, cos, sin)
#include <SDL2/SDL.h>  // Pour les fonctions SDL

// Game production.
// The player is a spatial ship, he is draw at the 3/4 down of the screen.
// Asteroids spawn at the 1/4 top of the screen and go down.
// The player can move with event key "left" and "right". When he does, the ship don't move but the asteroids do.
// The player can shoot with event key "space", the bullet go up. If the bullet hit an asteroid, the asteroid is destroyed and the player win a point.

// structure de coordonnées x, y
typedef struct {
    int x;
    int y;
} Point;
// structure de l'angle des mouvements sur l'axe x et y
typedef struct {
    float x;
    float y;
} Angle;
// structure du vaisseau spatial
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
// structure du bullet
typedef struct {
    Point position;
    Angle angle;
    int distance;
    int speed;
    int radius;
    int damage;
    int collisioning;
} Bullet;
// structure de l'asteroide
typedef struct {
    Point position;
    int radius;
    int health;
} Asteroid;

// surface du jeu 
Point game_surface = {1000000, 1000000};
// taille de la fenetre
Point wnd_size = {640, 480};
// spatial ship position
Point spatial_ship_screen_position = {320, 360};
// spatial ship information
SpatialShip spatial_ship = {0, 0, 5, 100, 0, {15, 30}, {500000, 500000}, {0, 1}};

#define SPATIAL_SHIP_SPEED 5
#define ASTEROID_VIEWING_DISTANCE 360
#define BULLET_COOLDOWN_MS 750
#define BULLET_LIFETIME 1200

// fonction pour retourner un nouvel objet asteroid, celui spawn dans une surface de 1000 * 1000
Point NewAsteroidPosition() {
    return (Point) {rand() % wnd_size.x, rand() % wnd_size.y};
}
// Fonction pour retourner un tableau de 100 astéroïdes
Asteroid* ChunkOfSpace(int bloc_position_y, int bloc_position_x) {
    int asteroid_per_bloc = 4;
    Asteroid* asteroids = malloc(asteroid_per_bloc * sizeof(Asteroid)); // Allouer de la mémoire pour 100 astéroïdes
    if (asteroids == NULL) {
        fprintf(stderr, "Allocation mémoire pour ChunkOfSpace a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < asteroid_per_bloc; i++) {
        Point position = NewAsteroidPosition();
        // printf("rand position x %d, y %d\n", position.x, position.y);
        Point real_position = {position.x + (bloc_position_x * wnd_size.x), position.y + (bloc_position_y * wnd_size.y)};
        // printf("real position x %d, y %d\n", real_position.x, real_position.y);
        asteroids[i].position = real_position;
        asteroids[i].radius = wnd_size.x/5;
        asteroids[i].health = 100;
    }
    return asteroids;
}
// Fonction pour retourner un tableau de tableaux d'astéroïdes (matrice)
Asteroid** lineOfSpace(int bloc_position_y) {
    int bloc_number = game_surface.x/wnd_size.x;
    Asteroid** line = malloc(bloc_number * sizeof(Asteroid*)); // Allouer de la mémoire pour 100 pointeurs vers des astéroïdes
    if (line == NULL) {
        fprintf(stderr, "Allocation mémoire pour lineOfSpace a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < bloc_number; i++) {
        line[i] = ChunkOfSpace(bloc_position_y, i);
    }
    return line;
}
// Fonction pour retourner un tableau de tableaux de tableaux d'astéroïdes (matrice)
Asteroid*** Space() {
    int lign_number = game_surface.y/wnd_size.y;
    Asteroid*** space = malloc(lign_number * sizeof(Asteroid**)); // Allouer de la mémoire pour 100 pointeurs vers des tableaux d'astéroïdes
    if (space == NULL) {
        fprintf(stderr, "Allocation mémoire pour Space a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < lign_number; i++) {
        space[i] = lineOfSpace(i);
        int bloc_number = game_surface.x/wnd_size.x * i * 4;
        // fprintf("lign %d maked, number of bloc is \n", i, bloc_number);dont work
        printf("lign %d maked, number of bloc is %d\n", i, bloc_number);

    }
    return space;
}
// Fonction pour libérer la mémoire allouée pour un tableau d'astéroïdes
void FreeSpace(Asteroid*** space) {
    int lign_number = game_surface.y/wnd_size.y;
    int bloc_number = game_surface.x/wnd_size.x;
    for (int i = 0; i < lign_number; i++) {
        for (int j = 0; j < bloc_number; j++) {
            free(space[i][j]); // Libérer chaque tableau d'astéroïdes individuellement
        }
        free(space[i]); // Libérer chaque tableau de bloc individuellement
    }
    free(space); // Libérer le tableau de pointeurs vers des tableaux d'astéroïdes
}




// fonction qui calcule le rayon d'affichage des asteroids en fonction de leur distance avec le vaisseau spatial.
int CalculateViewingDistance(Point position) {
    return (int) sqrt(pow(spatial_ship.position.x - position.x, 2) + pow(spatial_ship.position.y - position.y, 2));
}
int CalculDrawRadius(Point position, int radius) {
    float distance = CalculateViewingDistance(position);
    float ratio = (ASTEROID_VIEWING_DISTANCE - distance) / ASTEROID_VIEWING_DISTANCE;
    if (ratio < 0) ratio = 0; // Assurer que le ratio ne s'inverse pas
    radius = radius * ratio;
    return radius;
}
// calcule la position à l'écran d'une astorïde en fonction de l'angle directionnel du vaisseau spatial
Point CalculObjectScreenPosition(Point position) {
    // Calcul des deltas de position par rapport au vaisseau spatial
    float deltaX = (float)(position.x - spatial_ship.position.x);
    float deltaY = (float)(position.y - spatial_ship.position.y);
    // Application de l'angle de direction du vaisseau spatial pour obtenir les nouvelles positions
    float screenX = deltaX * spatial_ship.angle.x;
    float screenY = deltaY * spatial_ship.angle.y;
    // Conversion en coordonnées d'écran
    screenX += spatial_ship_screen_position.x;
    screenY += spatial_ship_screen_position.y;
    return (Point) { (int)screenX, (int)screenY };
}




// fonction de déplacement du vaisseau spatial
void moveSpatialShip(SpatialShip *ship) {
    ship->position.x += ship->speed * ship->angle.x;
    ship->position.y += ship->speed * ship->angle.y;
}
// fonction de rotation de +1° du vaisseau spatial
void rotateSpatialShipRight(SpatialShip *ship) {
    float angleInRadians = 1.0 * (3.14159 / 180); // 1 degré en radians
    float newX = ship->angle.x * cos(angleInRadians) - ship->angle.y * sin(angleInRadians);
    float newY = ship->angle.x * sin(angleInRadians) + ship->angle.y * cos(angleInRadians);
    ship->angle.x = newX;
    ship->angle.y = newY;
}
// fonction de rotation de -1° du vaisseau spatial
void rotateSpatialShipLeft(SpatialShip *ship) {
    float angleInRadians = -1.0 * (3.14159 / 180); // -1 degré en radians
    float newX = ship->angle.x * cos(angleInRadians) - ship->angle.y * sin(angleInRadians);
    float newY = ship->angle.x * sin(angleInRadians) + ship->angle.y * cos(angleInRadians);
    ship->angle.x = newX;
    ship->angle.y = newY;
}
// fonction de tir du vaisseau spatial
Bullet shoot(SpatialShip *ship) {
    Bullet bullet = {ship->position, ship->angle, 0, 10, wnd_size.x*0.05, 10, 0};
    return bullet;
}
Bullet* addBullet(Bullet* bullets, int bullet_size) {
    Bullet* newBullets = malloc(bullet_size * sizeof(Bullet));
    if (newBullets == NULL) {
        fprintf(stderr, "Allocation mémoire pour newBullets a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < bullet_size - 1; i++) {
        newBullets[i] = bullets[i];
    }
    free(bullets);
    Bullet b = shoot(&spatial_ship);
    newBullets[bullet_size - 1] = shoot(&spatial_ship);
    return newBullets;
}
Bullet* removeBullet(Bullet* bullets, int *bullet_size) {
    if (bullet_size == 0) {
        return NULL;
    }
    Bullet* newBullets = malloc(*bullet_size * sizeof(Bullet));
    if (newBullets == NULL) {
        fprintf(stderr, "Allocation mémoire pour newBullets a échoué.\n");
        return NULL;
    }
    int new_bullet_size = 0;
    for (int i = 0; i < *bullet_size; i++) {
        Bullet bullet = bullets[i];
        if (bullet.distance < BULLET_LIFETIME && bullet.collisioning < 60) {
            newBullets[i] = bullet;
            new_bullet_size++;
        } else {
            printf("?????? Bullet collisioning: %d, distance: %d\n", bullet.collisioning, bullet.distance);
        }
    }
    *bullet_size = new_bullet_size;
    free(bullets);
    if (new_bullet_size == 0) {
        return NULL;
    }
    return newBullets;
}
// fonction de déplacement du bullet
void moveBullet(Bullet *bullet) {
    bullet->position.x += bullet->speed * bullet->angle.x;
    bullet->position.y += bullet->speed * bullet->angle.y;
    bullet->distance += bullet->speed;
}
// fonction de collision entre le bullet et l'asteroide
bool CollisionBulletAsteroid(Bullet bullet, Asteroid asteroid) {
    int distance = sqrt(pow(bullet.position.x - asteroid.position.x, 2) + pow(bullet.position.y - asteroid.position.y, 2));
    return distance < bullet.radius + asteroid.radius;
}
// fonction de collision entre le vaisseau spatial et l'asteroide
bool CollisionSpatialShipAsteroid(SpatialShip ship, Asteroid asteroid) {
    int distance = sqrt(pow(ship.position.x - asteroid.position.x, 2) + pow(ship.position.y - asteroid.position.y, 2));
    return distance < ship.size.y + asteroid.radius;
}




// dessine un carré de 5 pixel reduit par la distance avec le vaisseau spatial
void DrawBullet(SDL_Renderer *renderer, Bullet bullet) {
    int radius = CalculDrawRadius(bullet.position, bullet.radius);
    Point bullet_screen_position = CalculObjectScreenPosition(bullet.position);
    if (bullet.collisioning > 0) {
        // dessiner un nuage de 20 rectangles aux positions aléatoire comprise dans un rayon de cinq fois le rayon du bullet.
        for (int i = 0; i < 20; i++) {
            int x = bullet_screen_position.x + (rand() % (radius * 10)) - (radius * 5);
            int y = bullet_screen_position.y + (rand() % (radius * 10)) - (radius * 5);
            SDL_Rect rect = {x, y, 2, 2};
            SDL_RenderFillRect(renderer, &rect);
        }
        return;
    }
    SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255); // orange foncé
    SDL_Rect rect = {bullet_screen_position.x - radius, bullet_screen_position.y - radius, radius * 2, radius * 2};
    SDL_RenderFillRect(renderer, &rect);
}
// draw a full circle for representing the asteroid
void DrawAsteroid(SDL_Renderer *renderer, Asteroid asteroid) {
    int radius = CalculDrawRadius(asteroid.position, asteroid.radius);
    Point asteroid_screen_position = CalculObjectScreenPosition(asteroid.position);
    SDL_SetRenderDrawColor(renderer, 139, 69, 19, 255); // marron
    for (int i = 0; i < 360; i++) {
        float x = asteroid_screen_position.x + radius * cos(i);
        float y = asteroid_screen_position.y + radius * sin(i);
        SDL_RenderDrawPoint(renderer, x, y);
    }
}
// draw a triangle for representing the spatial ship
void DrawSpatialShip(SDL_Renderer *renderer) {
    // Define the vertices of the spatial ship
    if (spatial_ship.collisioning > 0 && spatial_ship.collisioning % 2 == 0) {
        spatial_ship.collisioning++;
        if (spatial_ship.collisioning > 20) {
            spatial_ship.collisioning = 0;
        }
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255); // rouge
    } else {
        SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
    }
    SDL_Point vertices[4] = {
            {spatial_ship_screen_position.x, spatial_ship_screen_position.y},
            {spatial_ship_screen_position.x - 10, spatial_ship_screen_position.y + 20},
            {spatial_ship_screen_position.x + 10, spatial_ship_screen_position.y + 20},
            {spatial_ship_screen_position.x, spatial_ship_screen_position.y}
    };
    SDL_RenderDrawLines(renderer, vertices, 4);
}




// fonction retournant l'index des blocs de l'espace possédant des astéroïdes potentiellement affiché à l'écran
Point* GetVisibleBlocks(SpatialShip *ship) {
    Point* blocks = malloc(sizeof(Point) * 4);
    blocks[0] = (Point) {ship->position.x / wnd_size.x, ship->position.y / wnd_size.y};
    blocks[1] = (Point) {blocks[0].x + 1, blocks[0].y};
    blocks[2] = (Point) {blocks[0].x, blocks[0].y + 1};
    blocks[3] = (Point) {blocks[0].x + 1, blocks[0].y + 1};
    return blocks;
}
// fonction qui retourne les object de la scène à afficher
Asteroid* GetBlocks(SDL_Renderer *renderer, Asteroid*** asteroids, SpatialShip *ship) {
    Asteroid* scene_object = malloc(16 * sizeof(Asteroid));
    int count = 0;
    Point* blocks = GetVisibleBlocks(ship);
    for (int i = 0; i < 4; i++) {
        Point block = blocks[i];
        Asteroid* block_asteroids = asteroids[block.y][block.x];
        for (int j = 0; j < 4; j++) {
            Asteroid asteroid = block_asteroids[j];
            scene_object[count] = asteroid;
            count++;
        }
    }
    free(blocks);
    return scene_object;
}
// fonction qui renvoie un tableau d'astéroïde dont les positions sont les positions à l'écran des astéroïdes d'origine.
Asteroid* CalculAsteroidScreenPosition(Asteroid* scene_object) {
    Asteroid* scene_object_screen_position = malloc(16 * sizeof(Asteroid));
    for (int i = 0; i < 16; i++) {
        Asteroid asteroid = scene_object[i];
        scene_object_screen_position[i].position = CalculObjectScreenPosition(asteroid.position);
        scene_object_screen_position[i].radius = asteroid.radius;
    }
    free(scene_object);
    return scene_object_screen_position;
}
// fonction qui renvoie un tableau d'astéroïde après avoir trié celles ci par ordre croissant sur la position y.
Asteroid* SortAsteroidSceneObject(Asteroid* scene_object) {
    Asteroid* sorted_scene_object = malloc(16 * sizeof(Asteroid));
    if (!sorted_scene_object) {
        perror("Failed to allocate memory for sorted_scene_object");
        return NULL;
    }
    int count = 0;
    while (count < 16) {
        int min_index = -1;
        for (int i = 0; i < 16 - count; i++) {
            bool lower = true;
            Asteroid asteroid = scene_object[i];
            for (int j = 0; j < 16 - count; j++) {
                if (j == i) continue;
                if (asteroid.position.y > scene_object[j].position.y) {
                    lower = false;
                    break;
                }
            }
            if (lower) {
                //printf("lower value finded at i: %d, is y: %d, x: %d, count: %d\n", i, asteroid.position.y, asteroid.position.x, count);
                sorted_scene_object[count] = asteroid;
                count++;
                Asteroid* scene_object_remover = malloc((16 - count) * sizeof(Asteroid));
                if (!scene_object_remover) {
                    perror("Failed to allocate memory for scene_object_remover");
                    free(scene_object);
                    free(sorted_scene_object);
                    return NULL;
                }
                for (int j = 0, k = 0; j < 16 - count + 1; j++) {
                    if (j != i) {
                        scene_object_remover[k++] = scene_object[j];
                    }
                }
                free(scene_object);
                scene_object = scene_object_remover;
                break;
            }
        }
    }
    free(scene_object);
    return sorted_scene_object;
}
// fonction qui renvoie un tableau de bullet dont les positions sont les positions à l'écran des bullets d'origine.
Bullet* CalculBulletScreenPosition(Bullet* bullets, int bullet_size) {
    Bullet* bullets_screen_position = malloc(bullet_size * sizeof(Bullet));
    for (int i = 0; i < bullet_size; i++) {
        Bullet bullet = bullets[i];
        bullets_screen_position[i].position = CalculObjectScreenPosition(bullet.position);
    }
    return bullets_screen_position;
}
// fonction qui renvoie un tableau de bullet après avoir trié celles ci par ordre croissant sur la position y.
Bullet* SortBullet(Bullet* bullets, int bullet_size) {
    Bullet* sorted_bullets = malloc(16 * sizeof(Bullet));
    if (!sorted_bullets) {
        perror("Failed to allocate memory for sorted_bullets");
        return NULL;
    }
    int count = 0;
    for (;count < bullet_size;) {
        for (int i = 0; i < bullet_size - count; i++) {
            bool lower = true;
            Bullet bullet = bullets[i];
            for (int j=0; j < bullet_size - count; j++) {
                if (j == i) {
                    continue;
                }
                if (bullet.position.y > bullets[j].position.y) {
                    lower = false;
                    break;
                }
            }
            if (lower) {
                //printf("lower value finded at i: %d, is y: %d, x: %d, count: %d\n", i, bullet.position.y, bullet.position.x, count);
                sorted_bullets[count] = bullet;
                count++;
                if (count == bullet_size) {
                    break;
                }
                Bullet* bullets_remover = malloc((bullet_size - count) * sizeof(Bullet));
                if (!bullets_remover) {
                    perror("Failed to allocate memory for scene_object_remover");
                    free(bullets_remover);
                    free(sorted_bullets);
                    return NULL;
                }
                for (int j = 0; j < bullet_size - count; j++) {
                    if (j != i) {
                        if (j < i) {
                            bullets_remover[j] = bullets[j];
                        } else {
                            bullets_remover[j-1] = bullets[j];
                        }
                    }
                }
                free(bullets);
                bullets = bullets_remover;
            }
        }
    }
    free(bullets);
    return sorted_bullets;
}


//fonction qui gère la scène
void Scene(SDL_Renderer *renderer, Asteroid*** asteroids, Bullet* bull, int bullet_number, SpatialShip *ship) {
        if (bull != NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 10 vitesse: %d\n", bull[0].speed);
        }
    Asteroid* scene_object_asteroids = GetBlocks(renderer, asteroids, ship);
        if (bull != NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 11 vitesse: %d\n", bull[0].speed);
        }
    Asteroid* screen_position_asteroids = CalculAsteroidScreenPosition(scene_object_asteroids);
        if (bull != NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 12 vitesse: %d\n", bull[0].speed);
        }
    Asteroid* sorted_asteroids = SortAsteroidSceneObject(screen_position_asteroids);
    printf("asteroids sorted\n");
    
        if (bull != NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 100 vitesse: %d\n", bull[0].speed);
        }
    Bullet* screen_position_bullets = CalculBulletScreenPosition(bull, bullet_number);
        if (bull != NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 101 vitesse: %d\n", bull[0].speed);
        }
    Bullet* sorted_bullets = SortBullet(screen_position_bullets, bullet_number);
        if (bull != NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 102 vitesse: %d\n", bull[0].speed);
        }
    printf("bullets sorted\n");
    
    int bullets_index = 0;
    bool ship_drawed = false;
    for (int i = 0; i < 16; i++) {
        for (int j = bullets_index; j < bullet_number; j++) {
            if (sorted_bullets[j].position.y < sorted_asteroids[i].position.y) {
                DrawBullet(renderer, sorted_bullets[j]);
                bullets_index++;
            }
        }
        if (!ship_drawed && ship->position.y < sorted_asteroids[i].position.y) {
            DrawSpatialShip(renderer);
            ship_drawed = true;
        }
        DrawAsteroid(renderer, sorted_asteroids[i]);
    }
    for (int i = bullets_index; i < bullet_number; i++) {
        DrawBullet(renderer, sorted_bullets[i]);
    }

    free(sorted_asteroids);
    free(sorted_bullets);
}
// fonction qui gère les mouvements
void Move(Bullet* bullets, int bullet_size, SpatialShip *ship) {
    for (int i = 0; i < bullet_size; i++) {
        if (bullets[i].collisioning > 0) {
            continue;
        }
        moveBullet(&bullets[i]);
    }
    moveSpatialShip(ship);
}
// fonction qui gère les collisions
void Collision(Asteroid*** asteroids, Bullet* bullets, int bullet_number, SpatialShip *ship) {
    printf("collision bullet\n");
    for (int b= 0; b < bullet_number; b++) {
        if (bullets[b].collisioning > 0) {
            continue;;
        }
        int y = b / wnd_size.y;
        int x = b / wnd_size.x;
        Asteroid* arr_asteroid = asteroids[y][x];
        for (int i = 0; i < 4; i++) {
            if (CollisionBulletAsteroid(bullets[b], arr_asteroid[i])) {
                bullets[b].collisioning = 1;
            }
        }
    }
    printf("collision ship\n");
    if (ship->collisioning > 0) {
        return;
    }
    int y = ship->position.y / wnd_size.y;
    int x = ship->position.x / wnd_size.x;
    printf("asteroid selecting x : %d y : %d\n", x, y);
    Asteroid* arr_asteroid = asteroids[y][x];
    printf("asteroid selected\n");
    for (int i = 0; i < 4; i++) {
        if (CollisionSpatialShipAsteroid(*ship, arr_asteroid[i])) {
            ship->collisioning = 1;
        }
    }
}


int main(int argc, char *argv[]) {
    // GAME OBJECTS
    Bullet* bullets = NULL;
    int bullet_number = 0;
    Asteroid*** asteroids = Space();

    // SDL INIT
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Triangle Window",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          wnd_size.x,
                                          wnd_size.y,
                                          SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Event event;

    // RUNNING UTILITIES
    bool running = true;
    int last_bullet_shot = SDL_GetTicks();
    const int frameDelay = 1000 / 60;
    Uint32 frameStart;
    int frameTime;

    // SCREEN LOOP, IS DERECTLY CONTROLE THE PHYSIC OF THE GAME
    while (running) {
        // TIME MANAGEMENT
        frameStart = SDL_GetTicks();
        if (bullets != NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 1 vitesse: %d\n", bullets[0].speed);
        }
        // EVENT MANAGEMENT
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_LEFT) {
                rotateSpatialShipLeft(&spatial_ship);
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_RIGHT) {
                rotateSpatialShipRight(&spatial_ship);
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_SPACE) {
                if (SDL_GetTicks() - last_bullet_shot > 500) {
                    last_bullet_shot = SDL_GetTicks();
                    // Shoot a bullet
                    bullet_number++;
                    bullets = addBullet(bullets, bullet_number);
                }
            }
        }
        // PHYSIC
        if (bullets != NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 2 vitesse: %d\n", bullets[0].speed);
        }
        Move(bullets, bullet_number, &spatial_ship);
        if (bullets != NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 3 vitesse: %d\n", bullets[0].speed);
        }
        Collision(asteroids, bullets, bullet_number, &spatial_ship);
        if (bullets != NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 4 vitesse: %d\n", bullets[0].speed);
        }
        removeBullet(bullets, &bullet_number);
        if (bullets != NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 5 vitesse: %d\n", bullets[0].speed);
        }
        // DRAW
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Noir
        SDL_RenderClear(renderer);
        Scene(renderer, asteroids, bullets, bullet_number, &spatial_ship);
        if (bullets != NULL) {
            printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! 6 vitesse: %d\n", bullets[0].speed);
        }
        // FRAME RATE
        SDL_RenderPresent(renderer);
        frameTime = SDL_GetTicks() - frameStart;
        if (frameTime < frameDelay) {
            SDL_Delay(frameDelay - frameTime);
        }
    }

    free(bullets);
    FreeSpace(asteroids);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}

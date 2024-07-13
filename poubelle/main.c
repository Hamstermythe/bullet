#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <SDL2/SDL.h>

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

// Fonction pour retourner un tableau de 4 astéroïdes
Asteroid* ChunkOfSpace(int bloc_position_y, int bloc_position_x) {
    int asteroid_per_bloc = 4;
    Asteroid* asteroids = malloc(asteroid_per_bloc * sizeof(Asteroid));
    if (asteroids == NULL) {
        fprintf(stderr, "Allocation mémoire pour ChunkOfSpace a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < asteroid_per_bloc; i++) {
        Point position = NewAsteroidPosition();
        Point real_position = {position.x + (bloc_position_x * wnd_size.x), position.y + (bloc_position_y * wnd_size.y)};
        asteroids[i].position = real_position;
        asteroids[i].radius = wnd_size.x / 5;
        asteroids[i].health = 100;
    }
    return asteroids;
}

// Fonction pour retourner un tableau de tableaux d'astéroïdes (matrice)
Asteroid** lineOfSpace(int bloc_position_y) {
    int bloc_number = game_surface.x / wnd_size.x;
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

// Fonction pour retourner un tableau de tableaux de tableaux d'astéroïdes (matrice)
Asteroid*** Space() {
    int lign_number = game_surface.y / wnd_size.y;
    Asteroid*** space = malloc(lign_number * sizeof(Asteroid**));
    if (space == NULL) {
        fprintf(stderr, "Allocation mémoire pour Space a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < lign_number; i++) {
        space[i] = lineOfSpace(i);
        int bloc_number = game_surface.x / wnd_size.x * i * 4;
        printf("lign %d maked, number of bloc is %d\n", i, bloc_number);
    }
    return space;
}

// Fonction pour libérer la mémoire allouée pour un tableau d'astéroïdes
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

// fonction qui calcule le rayon d'affichage des asteroids en fonction de leur distance avec le vaisseau spatial.
int CalculateViewingDistance(Point position) {
    return (int)sqrt(pow(spatial_ship.position.x - position.x, 2) + pow(spatial_ship.position.y - position.y, 2));
}

// fonction pour calculer le rayon de dessin d'un objet en fonction de sa distance
int CalculDrawRadius(Point position, int radius) {
    float distance = CalculateViewingDistance(position);
    float ratio = (ASTEROID_VIEWING_DISTANCE - distance) / ASTEROID_VIEWING_DISTANCE;
    if (ratio < 0) ratio = 0;
    radius = radius * ratio;
    return radius;
}

// calcule la position à l'écran d'une astorïde en fonction de l'angle directionnel du vaisseau spatial
Point CalculObjectScreenPosition(Point position) {
    float deltaX = (float)(position.x - spatial_ship.position.x);
    float deltaY = (float)(position.y - spatial_ship.position.y);
    float screenX = deltaX * spatial_ship.angle.x;
    float screenY = deltaY * spatial_ship.angle.y;
    screenX += spatial_ship_screen_position.x;
    screenY += spatial_ship_screen_position.y;
    return (Point) { (int)screenX, (int)screenY };
}

// fonction de déplacement du vaisseau spatial
void moveSpatialShip(SpatialShip* ship) {
    ship->position.x += ship->speed * ship->angle.x;
    ship->position.y += ship->speed * ship->angle.y;
}

// fonction de rotation de +1° du vaisseau spatial
void rotateSpatialShipRight(SpatialShip* ship) {
    float angleInRadians = 1.0 * (3.14159 / 180);
    float newX = ship->angle.x * cos(angleInRadians) - ship->angle.y * sin(angleInRadians);
    float newY = ship->angle.x * sin(angleInRadians) + ship->angle.y * cos(angleInRadians);
    ship->angle.x = newX;
    ship->angle.y = newY;
}

// fonction de rotation de -1° du vaisseau spatial
void rotateSpatialShipLeft(SpatialShip* ship) {
    float angleInRadians = -1.0 * (3.14159 / 180);
    float newX = ship->angle.x * cos(angleInRadians) - ship->angle.y * sin(angleInRadians);
    float newY = ship->angle.x * sin(angleInRadians) + ship->angle.y * cos(angleInRadians);
    ship->angle.x = newX;
    ship->angle.y = newY;
}

// fonction de tir du vaisseau spatial
Bullet shoot(SpatialShip* ship) {
    Bullet bullet = {ship->position, ship->angle, 0, 10, wnd_size.x * 0.05, 10, 0};
    return bullet;
}

// ajouter un bullet
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
    newBullets[bullet_size - 1] = shoot(&spatial_ship);
    return newBullets;
}

// supprimer un bullet
Bullet* removeBullet(Bullet* bullets, int* bullet_size) {
    if (*bullet_size == 0) {
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
void moveBullet(Bullet* bullet) {
    bullet->position.x += bullet->speed * bullet->angle.x;
    bullet->position.y += bullet->speed * bullet->angle.y;
    bullet->distance += bullet->speed;
}

// fonction de collision entre le bullet et l'asteroide
bool collisionBulletAsteroid(Bullet bullet, Asteroid* asteroid) {
    int distance = CalculateViewingDistance(asteroid->position);
    if (distance <= bullet.radius + asteroid->radius) {
        asteroid->health -= bullet.damage;
        bullet.collisioning++;
        return true;
    }
    return false;
}

// fonction de collision entre le vaisseau spatial et l'asteroide
bool collisionSpatialShipAsteroid(SpatialShip ship, Asteroid* asteroid) {
    int distance = CalculateViewingDistance(asteroid->position);
    if (distance <= ship.size.x / 2 + asteroid->radius) {
        ship.health -= 10;
        ship.collisioning++;
        return true;
    }
    return false;
}

Point getBlocks(Point position) {
    return (Point) { position.x / wnd_size.x, position.y / wnd_size.y };
}

void sortAsteroid(Asteroid** asteroids, int asteroid_count) {
    for (int i = 0; i < asteroid_count - 1; i++) {
        for (int j = 0; j < asteroid_count - i - 1; j++) {
            if (CalculateViewingDistance(asteroids[j]->position) > CalculateViewingDistance(asteroids[j + 1]->position)) {
                Asteroid* temp = asteroids[j];
                asteroids[j] = asteroids[j + 1];
                asteroids[j + 1] = temp;
            }
        }
    }
}

void sortBullet(Bullet* bullets, int bullet_count) {
    for (int i = 0; i < bullet_count - 1; i++) {
        for (int j = 0; j < bullet_count - i - 1; j++) {
            if (bullets[j].distance > bullets[j + 1].distance) {
                Bullet temp = bullets[j];
                bullets[j] = bullets[j + 1];
                bullets[j + 1] = temp;
            }
        }
    }
}

void scene(Asteroid*** space, Bullet* bullets, int bullet_count, SDL_Renderer* renderer) {
    int lign_number = game_surface.y / wnd_size.y;
    int bloc_number = game_surface.x / wnd_size.x;
    for (int i = 0; i < bullet_count; i++) {
        moveBullet(&bullets[i]);
        Point screen_position = CalculObjectScreenPosition(bullets[i].position);
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        SDL_Rect bullet_rect = {screen_position.x, screen_position.y, bullets[i].radius, bullets[i].radius};
        SDL_RenderFillRect(renderer, &bullet_rect);
    }

    Point spatial_bloc = getBlocks(spatial_ship.position);
    for (int i = spatial_bloc.y - 1; i <= spatial_bloc.y + 1; i++) {
        for (int j = spatial_bloc.x - 1; j <= spatial_bloc.x + 1; j++) {
            if (i >= 0 && i < lign_number && j >= 0 && j < bloc_number) {
                Asteroid* asteroids = space[i][j];
                for (int k = 0; k < 4; k++) {
                    Asteroid *asteroid = &asteroids[k];
                    if (CalculateViewingDistance(asteroid->position) < ASTEROID_VIEWING_DISTANCE) {
                        Point screen_position = CalculObjectScreenPosition(asteroid->position);
                        int draw_radius = CalculDrawRadius(asteroid->position, asteroid->radius);
                        SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
                        SDL_Rect asteroid_rect = {screen_position.x - draw_radius / 2, screen_position.y - draw_radius / 2, draw_radius, draw_radius};
                        SDL_RenderFillRect(renderer, &asteroid_rect);

                        for (int l = 0; l < bullet_count; l++) {
                            if (collisionBulletAsteroid(bullets[l], asteroid)) {
                                bullets = removeBullet(bullets, &bullet_count);
                            }
                        }
                        if (collisionSpatialShipAsteroid(spatial_ship, asteroid)) {
                            printf("Collision avec un astéroïde! Santé du vaisseau: %d\n", spatial_ship.health);
                        }
                    }
                }
            }
        }
    }
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Spatial Game", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, wnd_size.x, wnd_size.y, 0);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, 0);
    
    bool running = true;
    SDL_Event event;
    Bullet* bullets = NULL;
    int bullet_size = 0;

    Asteroid*** space = Space();

    while (running) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            }
            if (event.type == SDL_KEYDOWN) {
                switch (event.key.keysym.sym) {
                    case SDLK_UP:
                        moveSpatialShip(&spatial_ship);
                        break;
                    case SDLK_LEFT:
                        rotateSpatialShipLeft(&spatial_ship);
                        break;
                    case SDLK_RIGHT:
                        rotateSpatialShipRight(&spatial_ship);
                        break;
                    case SDLK_SPACE:
                        bullet_size++;
                        bullets = addBullet(bullets, bullet_size);
                        spatial_ship.last_bullet_shot = SDL_GetTicks();
                        break;
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        scene(space, bullets, bullet_size, renderer);

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    FreeSpace(space);
    free(bullets);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    
    return 0;
}

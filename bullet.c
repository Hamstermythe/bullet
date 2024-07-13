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
    int r;
    int g;
    int b;
    int a;
} Color;

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

typedef struct {
    Point position;
    Angle angle;
    int distance;
    int speed;
    int radius;
    int damage;
    int collisioning;
} Bullet;

typedef struct {
    Point position;
    Color color;
    int radius;
    int health;
} Asteroid;

// Constants
#define SPATIAL_SHIP_SPEED 5
#define ASTEROID_VIEWING_DISTANCE 720
#define BULLET_COOLDOWN_MS 250
#define BULLET_LIFETIME 1200
#define VISIBLE_BLOCK_NUMBER 9
#define ASTEROID_PER_BLOC 1
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105

// Globals
Point wnd_size = {640, 480};
Point spatial_ship_screen_position = {320, 360};
Point game_surface = {0, 0}; //{1000000, 1000000};

// Function Declarations
Point NewAsteroidPosition();
Asteroid* ChunkOfSpace(int bloc_position_y, int bloc_position_x);
Asteroid** lineOfSpace(int bloc_position_y);
Asteroid*** Space();
void FreeSpace(Asteroid*** space);

int CalculateViewingDistance(Point position, SpatialShip spatial_ship);
int CalculDrawRadius(Point position, int radius, SpatialShip spatial_ship);
Point CalculObjectScreenPosition(Point position, SpatialShip spatial_ship);

Point gameSurfaceAntiDebordement(Point position);
void moveSpatialShip(SpatialShip *ship);
void rotateSpatialShipRight(SpatialShip *ship);
void rotateSpatialShipLeft(SpatialShip *ship);
Bullet shoot(SpatialShip ship);
Bullet* AddBullet(Bullet* bullets, int *bullet_size, SpatialShip spatial_ship);
Bullet* RemoveBullet(Bullet* bullets, int *bullet_size);
void moveBullets(Bullet* bullet, int bullet_size);
bool CollisionBulletAsteroid(Bullet bullet, Asteroid asteroid);
bool CollisionSpatialShipAsteroid(SpatialShip ship, Asteroid asteroid);
void Collision(Bullet* bullets, int bullet_size, Asteroid*** asteroids, SpatialShip *spatial_ship);
// void Move(Bullet* *bullets, int bullet_size, SpatialShip *ship);

void DrawBullet(SDL_Renderer *renderer, Bullet bullet);
void DrawAsteroid(SDL_Renderer *renderer, Asteroid asteroid);
void DrawSpatialShip(SDL_Renderer *renderer, SpatialShip spatial_ship);
void DrawMiniMap(SDL_Renderer *renderer, Asteroid*** asteroids, SpatialShip ship);

Point* GetVisibleBlocks(SpatialShip spatial_ship);
Asteroid* GetVisibleAsteroids(Asteroid*** asteroids, SpatialShip spatial_ship);
Asteroid* CalculAsteroidScreenPosition(Asteroid* scene_object, SpatialShip spatial_ship);
Asteroid* SortAsteroidSceneObject(Asteroid* scene_object);
Bullet* BulletsScreenPosition(Bullet* bullets, int bullet_size, SpatialShip spatial_ship);
Bullet* SortBullet(Bullet* bullets, int bullet_size);
void Scene(SDL_Renderer *renderer, Asteroid*** asteroids, Bullet* bullets, int bullet_number, SpatialShip ship);

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

    game_surface = (Point) {wnd_size.x * 100, wnd_size.y * 100};
    SpatialShip ship = {0, 0, SPATIAL_SHIP_SPEED, 100, 0, {20, 20}, {wnd_size.x, wnd_size.y}, {0, 1}};
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
                        rotateSpatialShipLeft(&ship);
                        printf("spatial_ship angle x: %f  y: %f\n", ship.angle.x, ship.angle.y);
                        break;
                    case SDLK_RIGHT:
                        rotateSpatialShipRight(&ship);
                        printf("spatial_ship angle x: %f  y: %f\n", ship.angle.x, ship.angle.y);
                        break;
                    case SDLK_UP:
                        moveSpatialShip(&ship);
                        printf("\nspatial_ship position x: %d  y: %d\n", ship.position.x, ship.position.y);
                        break;
                    case SDLK_SPACE:
                        if (SDL_GetTicks() - lastBulletShotTime > BULLET_COOLDOWN_MS) {
                            Bullet* temp = AddBullet(bullets, &bullet_size, ship);
                            if (temp != NULL) {
                                // FUITE
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

        //printf("spatial_ship angle x: %f  y: %f\n", ship.angle.x, ship.angle.y);
        //Move(&bullets, bullet_size, &ship);
        //moveSpatialShip(&ship);
        moveBullets(bullets, bullet_size);
        //printf("before collision\n");
        Collision(bullets, bullet_size, asteroids, &ship);
        //printf("Bullet size before: %d\n", bullet_size);
        Bullet* temp = RemoveBullet(bullets, &bullet_size);
        if (temp != NULL) {
            // FUITE
            bullets = realloc(temp, bullet_size * sizeof(Bullet));
        } else {
            bullets = NULL;
        }
        //printf("Bullet size after: %d\n", bullet_size);

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);
        Scene(renderer, asteroids, bullets, bullet_size, ship);
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
    return (Point) {0, 0}; //wnd_size.x / 2, wnd_size.y / 2};
    //return (Point) {rand() % wnd_size.x, rand() % wnd_size.y};
}
Color NewAsteroidColor() {
    return (Color) {(rand() % 150) + 105, (rand() % 150) + 105, (rand() % 150) + 105, 255};
}

Asteroid* ChunkOfSpace(int bloc_position_y, int bloc_position_x) {
    // FUITE
    Asteroid* asteroids = malloc(ASTEROID_PER_BLOC * sizeof(Asteroid));
    if (asteroids == NULL) {
        fprintf(stderr, "Allocation mémoire pour ChunkOfSpace a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < ASTEROID_PER_BLOC; i++) {
        Point position = NewAsteroidPosition();
        Color color = NewAsteroidColor();
        Point real_position = {position.x + (bloc_position_x * wnd_size.x), position.y + (bloc_position_y * wnd_size.y)};
        asteroids[i].position = real_position;
        asteroids[i].color = color;
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
    int lign_number = (game_surface.y / wnd_size.y) + 1;
    int bloc_number = (game_surface.x / wnd_size.x) + 1;
    for (int i = 0; i < lign_number; i++) {
        for (int j = 0; j < bloc_number; j++) {
            free(space[i][j]);
        }
        free(space[i]);
    }
    free(space);
}




// retourne la position adéquate du bout à bout de la surface de jeu.
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
// retourne la distance entre un point et le vaisseau spatial.
int CalculateViewingDistance(Point position, SpatialShip spatial_ship) {
    return (int)sqrt(pow(position.x - spatial_ship.position.x , 2) + pow(position.y - spatial_ship.position.y , 2));
}

int CalculDrawRadius(Point position, int radius, SpatialShip spatial_ship) {
    float distance = CalculateViewingDistance(position, spatial_ship);
    float ratio = (ASTEROID_VIEWING_DISTANCE - distance) / ASTEROID_VIEWING_DISTANCE;
    if (ratio < 0) ratio = 0;
    radius = radius * ratio;
    return radius;
}

Point CalculObjectScreenPosition(Point position, SpatialShip spatial_ship) {
    // Calcul des deltas de position par rapport au vaisseau spatial
    float deltaX = (float)(position.x - spatial_ship.position.x);
    float deltaY = (float)(position.y - spatial_ship.position.y);
    float distance = CalculateViewingDistance(position, spatial_ship);
    float angle_deg = atan2(deltaY, deltaX) * RAD_TO_DEG;
    angle_deg += 180;
    //float angle_vaisseau = atan2(spatial_ship.angle.y, spatial_ship.angle.x) * RAD_TO_DEG;
    //printf("angles : %f %f\n", angle_deg, angle_vaisseau);
    //angle_deg += angle_vaisseau;
    if (angle_deg > 360) angle_deg -= 360;
    if (angle_deg < 0) angle_deg += 360;
    angle_deg -= 180;
    float angle_x = cos(angle_deg * DEG_TO_RAD);
    float angle_y = sin(angle_deg * DEG_TO_RAD);
    float screenX = distance * angle_x;
    float screenY = distance * angle_y;
    /*
    // Application de l'angle de direction du vaisseau spatial pour obtenir les nouvelles positions
    float screenX = deltaX * spatial_ship.angle.x;
    float screenY = deltaY * spatial_ship.angle.y;
    // Conversion en coordonnées d'écran
    screenX = spatial_ship_screen_position.x - screenX;
    screenY = spatial_ship_screen_position.y - screenY;
    */
    screenX += spatial_ship_screen_position.x;
    screenY += spatial_ship_screen_position.y;
    return (Point) { (int)screenX, (int)screenY };
    //return position;
}




void moveSpatialShip(SpatialShip *ship) {
    ship->position.x += (int)(ship->angle.x * (float)(ship->speed));
    ship->position.y += (int)(ship->angle.y * (float)(ship->speed));
    ship->position = gameSurfaceAntiDebordement(ship->position);
}

void rotateSpatialShipRight(SpatialShip *ship) {
    float angle = atan2(ship->angle.y, ship->angle.x) * RAD_TO_DEG;
    angle -= 1 + 180;  // Rotate by 1 degree
    if (angle < 0) angle += 360;
    angle -= 180;
    angle *= DEG_TO_RAD;
    ship->angle.x = cos(angle);
    ship->angle.y = sin(angle);
}

void rotateSpatialShipLeft(SpatialShip *ship) {
    float angle = atan2(ship->angle.y, ship->angle.x) * RAD_TO_DEG;
    angle += 1 + 180;  // Rotate by 1 degree
    if (angle > 360) angle -= 360;
    angle -= 180;
    angle *= DEG_TO_RAD;
    ship->angle.x = cos(angle);
    ship->angle.y = sin(angle);
}

Bullet shoot(SpatialShip ship) {
    Bullet bullet = {ship.position, ship.angle, 0, 20, 10, 10, 0};
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

void moveBullets(Bullet* bullets, int bullet_size) {
    for (int i = 0; i < bullet_size; i++) {
        bullets[i].position.x += (int)(bullets[i].angle.x * (float)(bullets[i].speed));
        bullets[i].position.y += (int)(bullets[i].angle.y * (float)(bullets[i].speed));
        bullets[i].distance += bullets[i].speed;
        bullets[i].position = gameSurfaceAntiDebordement(bullets[i].position);
    }
    /*
    bullet->position.x += (int)(bullet->angle.x * (float)(bullet->speed));
    bullet->position.y += (int)(bullet->angle.y * (float)(bullet->speed));
    bullet->distance += bullet->speed;
    bullet->position = gameSurfaceAntiDebordement(bullet->position);
    */
}

bool CollisionBulletAsteroid(Bullet bullet, Asteroid asteroid) {
    float distance = sqrt(pow(asteroid.position.x - bullet.position.x, 2) + pow(asteroid.position.y - bullet.position.y, 2));
    return distance < asteroid.radius + bullet.radius;
}

bool CollisionSpatialShipAsteroid(SpatialShip ship, Asteroid asteroid) {
    float distance = sqrt(pow(asteroid.position.x - ship.position.x, 2) + pow(asteroid.position.y - ship.position.y, 2));
    return distance < asteroid.radius + ship.size.x / 2;
}

void Collision(Bullet* bullets, int bullet_size, Asteroid*** asteroids, SpatialShip *spatial_ship) {
    for (int i = 0; i < bullet_size; i++) {
        if (&bullets[i] == NULL) {
            continue;
        }
        int pos_y = bullets[i].position.y / wnd_size.y;
        int pos_x = bullets[i].position.x / wnd_size.x;
        Asteroid* arr_asteroid_a = asteroids[pos_y][pos_x];
        for (int j = 0; j < ASTEROID_PER_BLOC; j++) {
            if (CollisionBulletAsteroid(bullets[i], arr_asteroid_a[j])) {
                bullets[i].collisioning = 1;
                asteroids[pos_y][pos_x][j].health--;
                break;
            }
        }
    }
    int pos_y = spatial_ship->position.y / wnd_size.y;
    int pos_x = spatial_ship->position.x / wnd_size.x;
    Asteroid* arr_asteroid_b = asteroids[pos_y][pos_x];
    for (int i = 0; i < ASTEROID_PER_BLOC; i++) {
        if (CollisionSpatialShipAsteroid(*spatial_ship, arr_asteroid_b[i])) {
            spatial_ship->health--;
        }
    }
}

/*
void Move(Bullet* *bullets, int bullet_size, SpatialShip *spatial_ship) {
    moveSpatialShip(spatial_ship);
    for (int i = 0; i < bullet_size; i++) {
        moveBullet(bullets[i]);
    }
}
*/




void DrawBullet(SDL_Renderer *renderer, Bullet bullet) {
    if (bullet.collisioning > 0) {
        // dessiner un nuage de 20 rectangles aux positions aléatoire comprise dans un rayon de cinq fois le rayon du bullet.
        SDL_SetRenderDrawColor(renderer, 255, 140, 0, 255); // orange foncé
        for (int i = 0; i < 20; i++) {
            int x = bullet.position.x;// + (rand() % (radius * 10));// - (radius * 5);
            int y = bullet.position.y;// + (rand() % (radius * 10));// - (radius * 5);
            SDL_Rect rect = {x, y, 2, 2};
            SDL_RenderFillRect(renderer, &rect);
        }
        return;
    }
    SDL_Rect rect = {bullet.position.x - bullet.radius, bullet.position.y - bullet.radius, bullet.radius * 2, bullet.radius * 2};
    SDL_RenderFillRect(renderer, &rect);
}

void DrawAsteroid(SDL_Renderer *renderer, Asteroid asteroid) {
    SDL_SetRenderDrawColor(renderer, asteroid.color.r, asteroid.color.g, asteroid.color.b, asteroid.color.a);
    for (int w = 0; w < asteroid.radius * 2; w++) {
        for (int h = 0; h < asteroid.radius * 2; h++) {
            int dx = asteroid.radius - w;
            int dy = asteroid.radius - h;
            if ((dx * dx + dy * dy) <= (asteroid.radius * asteroid.radius)) {
                SDL_RenderDrawPoint(renderer, asteroid.position.x + dx, asteroid.position.y + dy);
            }
        }
    }
}

void DrawSpatialShip(SDL_Renderer *renderer, SpatialShip spatial_ship) {
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_Rect rect = {spatial_ship_screen_position.x - spatial_ship.size.x / 2, spatial_ship_screen_position.y - spatial_ship.size.y / 2, spatial_ship.size.x, spatial_ship.size.y};
    SDL_RenderFillRect(renderer, &rect);
    // viseur du vaisseau
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    int add_x = 20 * spatial_ship.angle.x;
    int add_y = 20 * spatial_ship.angle.y;
    int width = 5;
    SDL_Rect viseur = {spatial_ship_screen_position.x + add_x - width / 2, spatial_ship_screen_position.y + add_y - width / 2, width, width};
    SDL_RenderFillRect(renderer, &viseur);
}

void DrawMiniMap(SDL_Renderer *renderer, Asteroid*** asteroids, SpatialShip ship) {
    int size_x = wnd_size.x / 10;
    int size_y = wnd_size.y / 10;
    // dessiner un carré de 10% de la taille de la fenêtre
    SDL_Rect fond = {0, 0, size_x*3, size_y*3};
    SDL_SetRenderDrawColor(renderer, 50, 50, 50, 255);
    SDL_RenderFillRect(renderer, &fond);
    // dessiner les asteroides visibles
    int centre_x = (size_x / 2) + size_x;
    int centre_y = (size_y / 2) + size_y;
    Asteroid* visible_asteroid = GetVisibleAsteroids(asteroids, ship);
    for (int i = 0; i < VISIBLE_BLOCK_NUMBER * ASTEROID_PER_BLOC; i++) {
        int x = ((visible_asteroid[i].position.x - ship.position.x) / 10) + centre_x;
        int y = ((visible_asteroid[i].position.y - ship.position.y) / 10) + centre_y;
        SDL_Rect rect = {x-4, y-4, 8, 8};
        SDL_SetRenderDrawColor(renderer, visible_asteroid[i].color.r, visible_asteroid[i].color.g, visible_asteroid[i].color.b, visible_asteroid[i].color.a);
        SDL_RenderFillRect(renderer, &rect);
    }
    // dessiner un ligne suivant la direction du vaisseau
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawLine(renderer, centre_x, centre_y, centre_x + (ship.angle.x*8), centre_y + (ship.angle.y*8));
    // dessiner un carré pour la position du vaisseau
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_Rect rect = {centre_x - 2, centre_y - 2, 4, 4};
    SDL_RenderFillRect(renderer, &rect);
    free(visible_asteroid);
}




// retourne les index de colonne et de ligne des blocs visibles
Point* GetVisibleBlocks(SpatialShip spatial_ship) {
    int view_block_x = (spatial_ship.position.x / wnd_size.x);
    int view_block_y = (spatial_ship.position.y / wnd_size.y);
    Point* visible_blocks = malloc(VISIBLE_BLOCK_NUMBER * sizeof(Point));
    if (visible_blocks == NULL) {
        fprintf(stderr, "Allocation de mémoire pour les blocs visibles a échoué.\n");
        return NULL;
    }
    visible_blocks[0] = (Point){view_block_x - 1, view_block_y - 1};
    visible_blocks[1] = (Point){view_block_x, view_block_y - 1};
    visible_blocks[2] = (Point){view_block_x + 1, view_block_y - 1};
    visible_blocks[3] = (Point){view_block_x - 1, view_block_y};
    visible_blocks[4] = (Point){view_block_x, view_block_y};
    visible_blocks[5] = (Point){view_block_x + 1, view_block_y};
    visible_blocks[6] = (Point){view_block_x - 1, view_block_y + 1};
    visible_blocks[7] = (Point){view_block_x, view_block_y + 1};
    visible_blocks[8] = (Point){view_block_x + 1, view_block_y + 1};
    int max_x = game_surface.x / wnd_size.x;
    int max_y = game_surface.y / wnd_size.y;
    for (int i = 0; i < VISIBLE_BLOCK_NUMBER; i++) {
        if (visible_blocks[i].x < 0) {
            visible_blocks[i].x = max_x + visible_blocks[i].x;
        } else if (visible_blocks[i].x > max_x) {
            visible_blocks[i].x = visible_blocks[i].x - max_x;
        }
        if (visible_blocks[i].y < 0) {
            visible_blocks[i].y = max_y + visible_blocks[i].y;
        } else if (visible_blocks[i].y > max_y) {
            visible_blocks[i].y = visible_blocks[i].y - max_y;
        }
    }
    return visible_blocks;
}
// retournr la liste des astéroides visibles
Asteroid* GetVisibleAsteroids(Asteroid*** asteroids, SpatialShip spatial_ship) {
    Point* visible_blocks = GetVisibleBlocks(spatial_ship);
    Asteroid* visible_asteroids = malloc((VISIBLE_BLOCK_NUMBER * ASTEROID_PER_BLOC) * sizeof(Asteroid));
    int index = 0;
    for (int i = 0; i < VISIBLE_BLOCK_NUMBER; i++) {
        Point block = visible_blocks[i];
        int block_x = block.x;
        int block_y = block.y;
        Asteroid* asteroids_in_block = asteroids[block_y][block_x];
        for (int j = 0; j < ASTEROID_PER_BLOC; j++) {
            if (&asteroids_in_block[j] == NULL) {
                fprintf(stderr, "Reallocation de mémoire pour les astéroïdes visibles a échoué.\n");
                //return NULL;
                visible_asteroids[index] = (Asteroid) {(Point) {0, 0}, (Color) {100, 100, 100, 255}, 0, 0};
            }
            visible_asteroids[index] = (Asteroid) {asteroids_in_block[j].position, asteroids_in_block[j].color, asteroids_in_block[j].radius, asteroids_in_block[j].health};
            index++;
        }
    }
    free(visible_blocks);
    return visible_asteroids;
}

Asteroid* CalculAsteroidScreenPosition(Asteroid* scene_object, SpatialShip spatial_ship) {
    for (int i = 0; i < ASTEROID_PER_BLOC * VISIBLE_BLOCK_NUMBER; i++) {
        int radius = CalculDrawRadius(scene_object[i].position, scene_object[i].radius, spatial_ship);
        Point screen_position = CalculObjectScreenPosition(scene_object[i].position, spatial_ship);
        scene_object[i].position = screen_position;
        scene_object[i].radius = radius;
    }
    return scene_object;
}

Bullet* BulletsScreenPosition(Bullet* bullets, int bullet_size, SpatialShip spatial_ship) {
    Bullet* screen_bullets = malloc(bullet_size * sizeof(Bullet));
    for (int i = 0; i < bullet_size; i++) {
        Point screen_position = CalculObjectScreenPosition(bullets[i].position, spatial_ship);
        screen_bullets[i] = (Bullet){screen_position, bullets[i].angle, bullets[i].distance, bullets[i].speed, bullets[i].radius, bullets[i].damage, bullets[i].collisioning};
        //screen_bullets[i].position = screen_position;
    }
    return screen_bullets;
}

Asteroid* SortAsteroidSceneObject(Asteroid* scene_object) {
    // Simple bubble sort based on radius
    for (int i = 0; i < ASTEROID_PER_BLOC * VISIBLE_BLOCK_NUMBER; i++) {
        for (int j = i + 1; j < ASTEROID_PER_BLOC * VISIBLE_BLOCK_NUMBER; j++) {
            if (scene_object[i].position.y > scene_object[j].position.y) {
                Asteroid temp = scene_object[i];
                scene_object[i] = scene_object[j];
                scene_object[j] = temp;
            }
        }
    }
    return scene_object;
}

Bullet* SortBullet(Bullet* bullets, int bullet_size) {
    // Simple bubble sort based on distance
    for (int i = 0; i < bullet_size; i++) {
        for (int j = i + 1; j < bullet_size; j++) {
            if (bullets[j].position.y < bullets[i].position.y) {
                Bullet temp = bullets[i];
                bullets[i] = bullets[j];
                bullets[j] = temp;
            }
        }
    }
    return bullets;
}

void Scene(SDL_Renderer *renderer, Asteroid*** asteroids, Bullet* bullets, int bullet_number, SpatialShip ship) {
    Asteroid* visible_asteroids = GetVisibleAsteroids(asteroids, ship);
    visible_asteroids = CalculAsteroidScreenPosition(visible_asteroids, ship);
    visible_asteroids = SortAsteroidSceneObject(visible_asteroids);
    Bullet* screen_pos_bullets = BulletsScreenPosition(bullets, bullet_number, ship);
    screen_pos_bullets = SortBullet(screen_pos_bullets, bullet_number);
    int bullets_index = 0;
    bool ship_drawed = false;
    for (int i = 0; i < ASTEROID_PER_BLOC * VISIBLE_BLOCK_NUMBER; i++) {
        for (int j = bullets_index; j < bullet_number; j++) {
            if (screen_pos_bullets[j].position.y < visible_asteroids[i].position.y) {
                DrawBullet(renderer, screen_pos_bullets[j]);
                bullets_index++;
            }
        }
        int decalage = spatial_ship_screen_position.x - visible_asteroids[i].position.x;
        if (decalage < 0) decalage = -decalage;
        float ratio = 1.0 - ((float)(decalage) / (float)(visible_asteroids[i].radius));
        float radius_multiplier = 0.25 * ratio;
        if (!ship_drawed && visible_asteroids[i].position.y + visible_asteroids[i].radius * radius_multiplier > spatial_ship_screen_position.y) {
            printf("ratio: %f, radius_multiplier: %f\n", ratio, radius_multiplier);
            printf("Draw ship, asteroid y: %d, index: %d\n", visible_asteroids[i].position.y, i); 
            DrawSpatialShip(renderer, ship);
            ship_drawed = true;
        }
        DrawAsteroid(renderer, visible_asteroids[i]);
    }
    for (int i = bullets_index; i < bullet_number; i++) {
        DrawBullet(renderer, screen_pos_bullets[i]);
    }
    if (!ship_drawed) {
        DrawSpatialShip(renderer, ship);
    }
    DrawMiniMap(renderer, asteroids, ship);
    /*
    for (int i = 0; i < ASTEROID_PER_BLOC * VISIBLE_BLOCK_NUMBER; i++) {
        DrawAsteroid(renderer, visible_asteroids[i], ship);
    }
    for (int i = 0; i < bullet_number; i++) {
        DrawBullet(renderer, bullets[i], ship);
    }
    DrawSpatialShip(renderer, ship);
    */
    free(visible_asteroids);
    free(screen_pos_bullets);
}


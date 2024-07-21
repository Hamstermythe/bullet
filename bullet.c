#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>


// Structure Definitions
typedef struct {
    float x;
    float y;
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
    int rotation_x;
} SpatialShip;

typedef struct {
    Point position;
    Angle angle;
    int distance;
    int speed;
    int radius;
    int damage;
    int collisioning;
    Point collision;
} Bullet;

typedef struct {
    Point position;
    Color color;
    int radius;
    int health;
} Asteroid;

// Constants
#define SPATIAL_SHIP_SPEED 6
#define ASTEROID_VIEWING_DISTANCE 1280
#define BULLET_COOLDOWN_MS 250
#define BULLET_LIFETIME 1200
#define VISIBLE_BLOCK_NUMBER 9
#define ASTEROID_PER_BLOC 2
#define DEG_TO_RAD 0.017453292519943295769236907684886
#define RAD_TO_DEG 57.295779513082320876798154814105
#define BULLET_EXPLOSION_DURATION 24

// Globals
int appli_step = 0;
TTF_Font* font = NULL;
Point wnd_size = {1280, 960}; // {640, 480};
Point spatial_ship_screen_position = {640, 810}; // {360, 405};
Point game_surface = {0, 0}; // {1000000, 1000000};

// Function Declarations
int Appli_openning();
SDL_Texture* Appli_playing(SDL_Renderer *renderer, SDL_Event event, bool *running, Uint32 *lastBulletShotTime, Asteroid*** *asteroids, Bullet* *bullets, int *bullet_size, SpatialShip *ship);
int Appli_game_over();

Point NewAsteroidPosition();
Color NewAsteroidColor();
int NewAsteroidRadius();
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
bool CollisionBulletAsteroid(Bullet bullet, Asteroid asteroid, SpatialShip ship);
bool CollisionSpatialShipAsteroid(SpatialShip ship, Asteroid asteroid);
void Collision(Bullet* bullets, int bullet_size, Asteroid*** asteroids, SpatialShip *spatial_ship);
// void Move(Bullet* *bullets, int bullet_size, SpatialShip *ship);
void ReduceAsteroidHealth(Asteroid*** *asteroids);

void DrawBullet(SDL_Renderer *renderer, Bullet bullet);
void DrawAsteroid(SDL_Renderer *renderer, Asteroid asteroid);
void DrawSpatialShip(SDL_Renderer *renderer, SpatialShip *spatial_ship);
void DrawMiniMap(SDL_Renderer *renderer, Asteroid*** asteroids, SpatialShip ship);
SDL_Texture* DrawHUD(SDL_Renderer *renderer, SpatialShip ship);

Point* GetAdjacentBlocks(Point position);
Asteroid* GetVisibleAsteroids(Asteroid*** asteroids, SpatialShip spatial_ship);
Asteroid* CalculAsteroidScreenPosition(Asteroid* scene_object, SpatialShip spatial_ship);
Asteroid* SortAsteroidSceneObject(Asteroid* scene_object);
Bullet* BulletsScreenPosition(Bullet* bullets, int bullet_size, SpatialShip spatial_ship);
Bullet* SortBullet(Bullet* bullets, int bullet_size);
SDL_Texture* Scene(SDL_Renderer *renderer, Asteroid*** asteroids, Bullet* bullets, int bullet_number, SpatialShip *ship);

// Main Function
int main(int argc, char *argv[]) {
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            printf("Space Shooter\n");
            printf("A simple space shooter game\n");
            printf("Usage: ./space_shooter\n");
            return 0;
        }
    }
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }
    if (TTF_Init() == -1) {
        fprintf(stderr, "Échec de l'initialisation de SDL_ttf: %s\n", TTF_GetError());
        SDL_Quit();
        return 1;
    }
    SDL_Window* window = SDL_CreateWindow("Space Shooter", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, wnd_size.x, wnd_size.y, SDL_WINDOW_SHOWN);
    if (window == NULL) {
        fprintf(stderr, "Could not create window: %s\n", SDL_GetError());
        TTF_Quit();
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
    font = TTF_OpenFont("font/Ubuntu-MI.ttf", 24); // Remplacez par le chemin de votre police
    if (!font) {
        fprintf(stderr, "Error: %s\n", TTF_GetError());
        return 1;
    }

    game_surface = (Point) {wnd_size.x * 100, wnd_size.y * 100};
    SpatialShip ship = {0, 0, SPATIAL_SHIP_SPEED, 100, 0, {20, 20}, {wnd_size.x*20, wnd_size.y*20}, {0, -1}, 0};
    Bullet* bullets = NULL;
    int bullet_size = 0;
    Asteroid*** asteroids = Space();
    printf("Space created\n");

    Uint32 lastBulletShotTime = SDL_GetTicks();
    bool running = true;
    SDL_Event event;
    SDL_Texture* texture;
    printf("Game started\n");
    while (running) {
        SDL_DestroyTexture(texture);
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderClear(renderer);

        if (appli_step == 0) {
            appli_step = Appli_openning();
            appli_step = 1;
        } else if (appli_step == 1) {
            texture = Appli_playing(renderer, event, &running, &lastBulletShotTime, &asteroids, &bullets, &bullet_size, &ship);
        } else if (appli_step == 2) {
            appli_step = Appli_game_over();
        }

        SDL_RenderPresent(renderer);
        SDL_Delay(16);
    }

    printf("Game ended\n");
    FreeSpace(asteroids);
    free(bullets);
    printf("Space freed\n");

    TTF_CloseFont(font);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();
    printf("SDL closed\n");
    return 0;
}

int Appli_openning() {
    return 0;
}

SDL_Texture* Appli_playing(SDL_Renderer *renderer, SDL_Event event, bool *running, Uint32 *lastBulletShotTime, Asteroid*** *asteroids, Bullet* *bullets, int *bullet_size, SpatialShip *ship) {
    while (SDL_PollEvent(&event)) {
        if (event.type == SDL_QUIT) {
            *running = false;
        } else if (event.type == SDL_KEYDOWN) {
            switch (event.key.keysym.sym) {
                case SDLK_ESCAPE:
                    *running = false;
                    break;
                case SDLK_LEFT:
                    ship->rotation_x = -1;
                    //printf("spatial_ship angle x: %f  y: %f\n", ship->angle.x, ship->angle.y);
                    break;
                case SDLK_RIGHT:
                    ship->rotation_x = 1;
                    //printf("spatial_ship angle x: %f  y: %f\n", ship->angle.x, ship->angle.y);
                    break;
                case SDLK_UP:
                    moveSpatialShip(ship);
                    //printf("\nspatial_ship position x: %f  y: %f\n", ship->position.x, ship->position.y);
                    break;
                case SDLK_SPACE:
                    if (SDL_GetTicks() - *lastBulletShotTime > BULLET_COOLDOWN_MS) {
                        *bullets = AddBullet(*bullets, bullet_size, *ship);// cause a leak in AddBullet function. Why?
                        // Bullet* temp = AddBullet(*bullets, bullet_size, *ship);
                        // if (temp != NULL) {
                            // FUITE
                            // *bullets = realloc(temp, *bullet_size * sizeof(Bullet));
                        // } else {
                            // *bullets = NULL;
                        // }
                        // free(temp); cause a crash because bullets become empty. Why a laek is generated on bullets reallocation?
                        *lastBulletShotTime = SDL_GetTicks();
                    }
                    break;
            }
        } else if (event.type == SDL_KEYUP) {
            switch (event.key.keysym.sym) {
                case SDLK_LEFT:
                    if (ship->rotation_x == -1) {
                        ship->rotation_x = 0;
                    }
                    break;
                case SDLK_RIGHT:
                    if (ship->rotation_x == 1) {
                        ship->rotation_x = 0;
                    }
                    break;
            }
        }
    }
    rotateSpatialShipRight(ship);
    rotateSpatialShipLeft(ship);
    moveSpatialShip(ship);
    moveBullets(*bullets, *bullet_size);
    Collision(*bullets, *bullet_size, *asteroids, ship);
    *bullets = RemoveBullet(*bullets, bullet_size);// cause a leak in RemoveBullet function. Why?
    // Bullet* temp = RemoveBullet(*bullets, bullet_size);
    // if (temp != NULL) {
        // FUITE
        // *bullets = realloc(temp, *bullet_size * sizeof(Bullet));
    // } else {
        // *bullets = NULL;
    // }
    // free(temp); cause a crash because bullets become empty. Why a laek is generated on bullets reallocation?
    SDL_Texture* texture = Scene(renderer, *asteroids, *bullets, *bullet_size, ship);
    ReduceAsteroidHealth(asteroids);
    /*
    if (ship->health <= -(float)(BULLET_EXPLOSION_DURATION)) {
        appli_step 2;
    }
    */
    return texture;
}

int Appli_game_over() {
    return 2;
}


// Implementations of previously declared functions
Point NewAsteroidPosition() {
    //  debug values
    // return (Point) {0, 0}; //wnd_size.x / 2, wnd_size.y / 2}; 
    return (Point) {rand() % (int)(wnd_size.x), rand() % (int)(wnd_size.y)};
}
Color NewAsteroidColor() {
    return (Color) {(rand() % 150) + 105, (rand() % 150) + 105, (rand() % 150) + 105, 255};
}
int NewAsteroidRadius() {
    int max_radius = (float)(wnd_size.x / 6);
    int rand_size = (float)(max_radius) * 0.9;
    int min_radius = (float)(max_radius) * 0.1;
    return (rand() % rand_size) + min_radius;
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
        int radius = NewAsteroidRadius();
        Point real_position = {position.x + (bloc_position_x * wnd_size.x), position.y + (bloc_position_y * wnd_size.y)};
        asteroids[i].position = real_position;
        asteroids[i].color = color;
        asteroids[i].radius = radius;
        asteroids[i].health = 100;
    }
    return asteroids;
}

Asteroid** lineOfSpace(int bloc_position_y) {
    int bloc_number = (game_surface.x / wnd_size.x);
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
    int lign_number = (game_surface.y / wnd_size.y);
    Asteroid*** space = malloc(lign_number * sizeof(Asteroid**));
    if (space == NULL) {
        fprintf(stderr, "Allocation mémoire pour Space a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < lign_number; i++) {
        space[i] = lineOfSpace(i);
        //int asteroid_number = game_surface.x / wnd_size.x * i * ASTEROID_PER_BLOC;
        //printf("lign %d maked, number of asteroid is %d\n", i, asteroid_number);
    }
    return space;
}

void FreeSpace(Asteroid*** space) {
    int lign_number = (game_surface.y / wnd_size.y);
    int bloc_number = (game_surface.x / wnd_size.x);
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
    } else if (position.x >= game_surface.x) {
        position.x = position.x - game_surface.x;
    }
    if (position.y < 0) {
        position.y = position.y + game_surface.y;
    } else if (position.y >= game_surface.y) {
        position.y = position.y - game_surface.y;
    }
    return position;
}
// retourne la distance entre un point et le vaisseau spatial
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
// calcul la position à l'écran d'un objet en fonction de la rotation du vaisseau spatial, celui-ci pointant toujours vers le haut de l'écran
Point CalculObjectScreenPosition(Point position, SpatialShip spatial_ship) {
    // Calcul des deltas de position par rapport au vaisseau spatial
    float deltaX = (float)(position.x - spatial_ship.position.x);
    float deltaY = (float)(position.y - spatial_ship.position.y);
    float distance = CalculateViewingDistance(position, spatial_ship);
    float angle_deg = atan2(deltaY, deltaX) * RAD_TO_DEG;
    angle_deg += 180;
    float angle_vaisseau = atan2(spatial_ship.angle.y, spatial_ship.angle.x) * RAD_TO_DEG;
    //printf("angles : %f %f\n", angle_deg, angle_vaisseau);
    angle_deg -= angle_vaisseau + 90;
    if (angle_deg > 360) angle_deg -= 360;
    if (angle_deg < 0) angle_deg += 360;
    angle_deg -= 180;
    float angle_x = cos(angle_deg * DEG_TO_RAD);
    float angle_y = sin(angle_deg * DEG_TO_RAD);
    float screenX = distance * angle_x;
    float screenY = distance * angle_y;
    screenX += spatial_ship_screen_position.x;
    screenY += spatial_ship_screen_position.y;
    return (Point) { (int)screenX, (int)screenY };
    //return position;
}




void moveSpatialShip(SpatialShip *ship) {
    if (ship->health <= 0) return;
    ship->position.x += (int)(ship->angle.x * (float)(ship->speed));
    ship->position.y += (int)(ship->angle.y * (float)(ship->speed));
    ship->position = gameSurfaceAntiDebordement(ship->position);
}

void rotateSpatialShipRight(SpatialShip *ship) {
    if (ship->rotation_x != 1) {
        return;
    }
    float angle = atan2(ship->angle.y, ship->angle.x) * RAD_TO_DEG;
    angle += 1 + 180;  // Rotate by 1 degree
    if (angle < 0) angle += 360;
    angle -= 180;
    angle *= DEG_TO_RAD;
    ship->angle.x = cos(angle);
    ship->angle.y = sin(angle);
}

void rotateSpatialShipLeft(SpatialShip *ship) {
    if (ship->rotation_x != -1) {
        return;
    }
    float angle = atan2(ship->angle.y, ship->angle.x) * RAD_TO_DEG;
    angle -= 1 + 180;  // Rotate by 1 degree
    if (angle > 360) angle -= 360;
    angle -= 180;
    angle *= DEG_TO_RAD;
    ship->angle.x = cos(angle);
    ship->angle.y = sin(angle);
}

Bullet shoot(SpatialShip ship) {
    Bullet bullet = (Bullet) {ship.position, ship.angle, 0, 20, 5, 10, 0, (Point) {0, 0}};
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
        if (bullets[i].distance < BULLET_LIFETIME && bullets[i].collisioning < BULLET_EXPLOSION_DURATION) {
            new_bullet_size++;
        }
    }
    if (new_bullet_size == 0) {
        *bullet_size = 0;
        return NULL;
    }
    Bullet* new_bullets = malloc(new_bullet_size * sizeof(Bullet));
    if (new_bullets == NULL) {
        fprintf(stderr, "Reallocation de mémoire pour les balles a échoué.\n");
        return NULL;
    }
    int n = 0;
    for (int i = 0; i < *bullet_size; i++) {
        if (bullets[i].distance < BULLET_LIFETIME && bullets[i].collisioning < BULLET_EXPLOSION_DURATION) {
            new_bullets[n] = bullets[i];
            n++;
        }
    }
    (*bullet_size) = new_bullet_size;
    return new_bullets;
}

void moveBullets(Bullet* bullets, int bullet_size) {
    for (int i = 0; i < bullet_size; i++) {
        // écrire dans le terminal la structure du bullet traité
        if (bullets[i].collisioning > 0) {
            bullets[i].collisioning += 1;
            continue;
        }
        bullets[i].position.x += (int)(bullets[i].angle.x * (float)(bullets[i].speed));
        bullets[i].position.y += (int)(bullets[i].angle.y * (float)(bullets[i].speed));
        bullets[i].distance += bullets[i].speed;
        bullets[i].position = gameSurfaceAntiDebordement(bullets[i].position);
    }
}

/* test de calibration pour que l'ellipse vérifie correctement lorsque le vaisseau est sur une diagonale
bool CollisionBulletAsteroid(Bullet bullet, Asteroid asteroid, SpatialShip spatial_ship) {
    int dist_with_ship = sqrt(pow(asteroid.position.x - spatial_ship.position.x, 2) + pow(asteroid.position.y - spatial_ship.position.y, 2));
    float ratio_viewing_dist = 1.0 - ((float)dist_with_ship / (float)ASTEROID_VIEWING_DISTANCE);
    if (ratio_viewing_dist <= 0) ratio_viewing_dist = 0;
    int dist_bullet_asteroid = sqrt(pow(bullet.position.x - asteroid.position.x, 2) + pow(bullet.position.y - asteroid.position.y, 2));
    bullet.position.y = asteroid.position.y + dist_bullet_asteroid;
    bullet.position.x = asteroid.position.x;
    float radius_multiplier_y = 0.25;// 0.25 + (0.75 * fabs(spatial_ship.angle.x));
    float radius_multiplier_x = 1.0;// 0.25 + (0.75 * fabs(spatial_ship.angle.y));
    // calculer les rayons x et y de l'ellipse
    int radius_asteroid_y = (float)(asteroid.radius) * (float)(ratio_viewing_dist) * (float)(radius_multiplier_y);
    int radius_asteroid_x = (float)(asteroid.radius) * (float)(ratio_viewing_dist) * (float)(radius_multiplier_x);
    float dx = bullet.position.x - (asteroid.position.x);
    float dy = bullet.position.y - asteroid.position.y;

    if (((dx * dx) / (radius_asteroid_x * radius_asteroid_x)) + ((dy * dy) / (radius_asteroid_y * radius_asteroid_y)) <= 1) {
        return true;
    }
    return false;
}
*/

bool CollisionBulletAsteroid(Bullet bullet, Asteroid asteroid, SpatialShip spatial_ship) {
    int dist_with_ship = sqrt(pow(asteroid.position.x - spatial_ship.position.x, 2) + pow(asteroid.position.y - spatial_ship.position.y, 2));
    float ratio_viewing_dist = 1.0 - ((float)dist_with_ship / (float)ASTEROID_VIEWING_DISTANCE);
    if (ratio_viewing_dist <= 0) ratio_viewing_dist = 0;
    // pivoter l'axe de l'ellipse en fonction de l'angle du vaisseau
    float radius_multiplier_y = 0.25 + (0.75 * fabs(spatial_ship.angle.x));
    float radius_multiplier_x = 0.25 + (0.75 * fabs(spatial_ship.angle.y));
    // calculer les rayons x et y de l'ellipse
    int radius_asteroid_y = (float)(asteroid.radius) * (float)(ratio_viewing_dist) * (float)(radius_multiplier_y);
    int radius_asteroid_x = (float)(asteroid.radius) * (float)(ratio_viewing_dist) * (float)(radius_multiplier_x);
    float dx = bullet.position.x - (asteroid.position.x);
    float dy = bullet.position.y - asteroid.position.y;
    if (((dx * dx) / (radius_asteroid_x * radius_asteroid_x)) + ((dy * dy) / (radius_asteroid_y * radius_asteroid_y)) <= 1) {
        return true;
    }
    return false;
}

bool CollisionSpatialShipAsteroid(SpatialShip spatial_ship, Asteroid asteroid) {
    int dist_with_ship = sqrt(pow(asteroid.position.x - spatial_ship.position.x, 2) + pow(asteroid.position.y - spatial_ship.position.y, 2));
    float ratio_viewing_dist = 1.0 - ((float)dist_with_ship / (float)ASTEROID_VIEWING_DISTANCE);
    if (ratio_viewing_dist <= 0) ratio_viewing_dist = 0;
    // pivoter l'axe de l'ellipse en fonction de l'angle du vaisseau
    float radius_multiplier_y = 0.25 + (0.75 * fabs(spatial_ship.angle.x));
    float radius_multiplier_x = 0.25 + (0.75 * fabs(spatial_ship.angle.y));
    // calculer les rayons x et y de l'ellipse
    int radius_asteroid_y = (float)(asteroid.radius + (spatial_ship.size.y / 2)) * (float)(ratio_viewing_dist) * (float)(radius_multiplier_y);
    int radius_asteroid_x = (float)(asteroid.radius + (spatial_ship.size.x / 2)) * (float)(ratio_viewing_dist) * (float)(radius_multiplier_x);
    float dx = spatial_ship.position.x - asteroid.position.x;
    float dy = spatial_ship.position.y - asteroid.position.y;
    if (((dx * dx) / (radius_asteroid_x * radius_asteroid_x)) + ((dy * dy) / (radius_asteroid_y * radius_asteroid_y)) <= 1) {
        return true;
    }
    return false;
}

void Collision(Bullet* bullets, int bullet_size, Asteroid*** asteroids, SpatialShip *spatial_ship) {
    for (int i = 0; i < bullet_size; i++) {
        if (bullets[i].collisioning > 0) {
            continue;
        }
        Point* arr_block = GetAdjacentBlocks(bullets[i].position);
        for (int block_pos = 0; block_pos < VISIBLE_BLOCK_NUMBER; block_pos++) {
            Asteroid* arr_asteroid_a = asteroids[(int)(arr_block[block_pos].y)][(int)(arr_block[block_pos].x)];
            for (int j = 0; j < ASTEROID_PER_BLOC; j++) {
                if (arr_asteroid_a[j].health <= 0) {
                    continue;
                }
                // vérifier pixel par pixel si le bullet touche un astéroïde, la taille d'un mouvement pouvant dépasser la taille de la hitbox
                Point originale_pos = (Point) {bullets[i].position.x, bullets[i].position.y};
                for (int m = 0; m < bullets[i].speed; m+=1) {
                    float add_x = bullets[i].angle.x * (float)(m);
                    float add_y = bullets[i].angle.y * (float)(m);
                    bullets[i].position.x = originale_pos.x + add_x;
                    bullets[i].position.y = originale_pos.y + add_y;
        /* don't work, the bullet can't touch asteroid if he is on the other side of the game surface, this even if bullet position become in the same block as the asteroid

        int tcheck_dist_x = (bullets[i].position.x - arr_asteroid_a[j].position.x);
        if (abs(tcheck_dist_x) > wnd_size.x * 2) {
            if (tcheck_dist_x > 0) {
                bullets[i].position.x -= game_surface.x;
            } else {
                bullets[i].position.x += game_surface.x;
            }
        }
        int tcheck_dist_y = (bullets[i].position.y - arr_asteroid_a[j].position.y);
        if (abs(tcheck_dist_y) > wnd_size.y * 2) {
            printf("tcheck_dist_y: %d, bullet %d y: %f, asteroid y: %f\n",tcheck_dist_y, i, bullets[i].position.y, arr_asteroid_a[j].position.y);
            if (tcheck_dist_y > 0) {
                bullets[i].position.y -= game_surface.y;
            } else {
                bullets[i].position.y += game_surface.y;
            }
        }
        //printf("bullet x: %f, y: %f, asteroid x: %f, y: %f\n", bullets[i].position.x, bullets[i].position.y, arr_asteroid_a[j].position.x, arr_asteroid_a[j].position.y);
        */
                    if (CollisionBulletAsteroid(bullets[i], arr_asteroid_a[j], *spatial_ship)) {
                        bullets[i].collisioning = 1;
                        bullets[i].collision = arr_asteroid_a[j].position;
                        arr_asteroid_a[j].health -= bullets[i].damage;
                        if (arr_asteroid_a[j].health == 0) {
                            spatial_ship->score += 1;
                        }
                        break;
                    }
                }
                // si le bullet n'a pas touché, lui rendre sa position originale pour qu'il puisse continuer son mouvement
                //if (bullets[i].collisioning == 0) {    // condition bug, don't work, cause the bullet stand as the position previously calculated.
                    bullets[i].position = originale_pos;
                //}
            }
        }
        free(arr_block);
    }
    int pos_y = spatial_ship->position.y / wnd_size.y;
    int pos_x = spatial_ship->position.x / wnd_size.x;
    Asteroid* arr_asteroid_b = asteroids[pos_y][pos_x];
    for (int i = 0; i < ASTEROID_PER_BLOC; i++) {
        if (arr_asteroid_b[i].health <= 0 || spatial_ship->health <= 0) {
            continue;
        }
        if (CollisionSpatialShipAsteroid(*spatial_ship, arr_asteroid_b[i])) {
            spatial_ship->health--;
        }
    }
}

// fonction servant à l'animation de l'explosion d'un astéroïde
void ReduceAsteroidHealth(Asteroid*** *asteroids) {
    int max_x = game_surface.x / wnd_size.x;
    int max_y = game_surface.y / wnd_size.y;
    for (int i = 0; i < max_y; i++) {
        for (int j = 0; j < max_x; j++) {
            for (int k = 0; k < ASTEROID_PER_BLOC; k++) {
                if ((*asteroids)[i][j][k].health <= 0 && (*asteroids)[i][j][k].health > -BULLET_EXPLOSION_DURATION) {
                    (*asteroids)[i][j][k].health--;
                }
            }
        }
    }
}



// dessine les projectiles
void DrawBullet(SDL_Renderer *renderer, Bullet bullet) {
    if (bullet.collisioning > 0) {
        // dessiner un nuage de 40 rectangles de couleur rouge ou gris, de 2 pixels de rayon, aux positions aléatoire comprise dans un rayon de 2 fois le rayon du bullet.*
        int radius = (float)((float)(bullet.radius) * 3) * (float)((float)(bullet.collisioning) / (float)(BULLET_EXPLOSION_DURATION));
        int radius_square = radius * radius;
        int center = radius_square / 4;
        int middle = center * 2;
        for (int w = 0; w < radius * 2; w++) {
            for (int h = 0; h < radius * 2; h++) {
                int dx = radius - w;
                int dy = radius - h;
                int square_dist = dx * dx + dy * dy;
                if (rand() % 3 == 0) {
                    if (square_dist > center && square_dist <= middle) {
                        SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
                        SDL_RenderDrawPoint(renderer, bullet.position.x + dx, bullet.position.y + dy);
                    } else if (square_dist <= radius_square && square_dist > middle && bullet.collisioning < (BULLET_EXPLOSION_DURATION * 7) / 8) {
                        SDL_SetRenderDrawColor(renderer, 250, 100, 0, 255);
                        SDL_RenderDrawPoint(renderer, bullet.position.x + dx, bullet.position.y + dy);
                    }
                }
            }
        }
        return;
    }
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    for (int w = 0; w < bullet.radius * 2; w++) {
        for (int h = 0; h < bullet.radius * 2; h++) {
            int dx = bullet.radius - w;
            int dy = bullet.radius - h;
            if ((dx * dx + dy * dy) <= (bullet.radius * bullet.radius)) {
                SDL_RenderDrawPoint(renderer, bullet.position.x + dx, bullet.position.y + dy);
            }
        }
    }
}

// dessine les astéroïdes
void DrawAsteroid(SDL_Renderer *renderer, Asteroid asteroid) {
    if (asteroid.health < -BULLET_EXPLOSION_DURATION) {
        return;
    }
    if (asteroid.health <= 0) {
        // explosion du vaisseau
        // dessiner un nuage de 40 rectangles de couleur rouge ou gris, de 2 pixels de rayon, aux positions aléatoire comprise dans un rayon de 2 fois le rayon du bullet.*
        int radius = (float)((asteroid.radius * 3) / 2) * (float)(abs(asteroid.health) / (float)(BULLET_EXPLOSION_DURATION));
        int radius_square = radius * radius;
        int center = radius_square / 4;
        int middle = center * 2;
        for (int w = 0; w < radius * 2; w++) {
            for (int h = 0; h < radius * 2; h++) {
                int dx = radius - w;
                int dy = radius - h;
                int square_dist = dx * dx + dy * dy;
                if (rand() % 3 == 0) {
                    if (square_dist > radius_square) {
                        continue;
                    }
                    if (square_dist > center && square_dist <= middle && abs(asteroid.health) < (BULLET_EXPLOSION_DURATION)) {
                        SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
                        SDL_RenderDrawPoint(renderer, asteroid.position.x + dx, asteroid.position.y + dy);
                    } else if (square_dist > middle && abs(asteroid.health) < (BULLET_EXPLOSION_DURATION * 7) / 8) {
                        SDL_SetRenderDrawColor(renderer, 250, 100, 0, 255);
                        SDL_RenderDrawPoint(renderer, asteroid.position.x + dx, asteroid.position.y + dy);
                    }
                }
            }
        }
    } else {
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
}

// dessine le vaisseau
void DrawSpatialShip(SDL_Renderer *renderer, SpatialShip *spatial_ship) {
    if (spatial_ship->health <= 0) {
        // explosion du vaisseau
        // dessiner un nuage de 40 rectangles de couleur rouge ou gris, de 2 pixels de rayon, aux positions aléatoire comprise dans un rayon de 2 fois le rayon du bullet.*
        int radius = (float)(spatial_ship->size.x * 4) * (float)(abs(spatial_ship->health) / (float)(BULLET_EXPLOSION_DURATION));
        int radius_square = radius * radius;
        int center = radius_square / 4;
        int middle = center * 2;
        for (int w = 0; w < radius * 2; w++) {
            for (int h = 0; h < radius * 2; h++) {
                int dx = radius - w;
                int dy = radius - h;
                int square_dist = dx * dx + dy * dy;
                if (rand() % 3 == 0) {
                    if (square_dist > radius_square) {
                        continue;
                    }
                    //if (square_dist > center && square_dist <= middle) {
                    if (square_dist > center && square_dist <= middle && abs(spatial_ship->health) < (BULLET_EXPLOSION_DURATION)) {
                        SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
                        SDL_RenderDrawPoint(renderer, spatial_ship_screen_position.x + dx, spatial_ship_screen_position.y + dy);
                    //} else if (square_dist <= radius_square && square_dist > middle && abs(spatial_ship->health) < (BULLET_EXPLOSION_DURATION * 7) / 8) {
                    } else if (square_dist > middle && abs(spatial_ship->health) < (BULLET_EXPLOSION_DURATION * 7) / 8) {
                        SDL_SetRenderDrawColor(renderer, 250, 100, 0, 255);
                        SDL_RenderDrawPoint(renderer, spatial_ship_screen_position.x + dx, spatial_ship_screen_position.y + dy);
                    }
                }
            }
        }
        spatial_ship->health--;
    } else {
        // dessiner un triangle à la largeur du vaisseau lorsque rotation_x vaut 0, et de demi-largeur lorsque rotation_x vaut 1 ou -1
        // si la rotationest vers la gauche la moitié gauche du vaiseau est plus foncé que la moitié droite, et inversement
        int start_y = spatial_ship_screen_position.y - (spatial_ship->size.y / 2);
        for (int y = 0; y < spatial_ship->size.y; y++) {
            int width = (float)(spatial_ship->size.x) * ((float)((float)(y) / (float)(spatial_ship->size.y)));
            if (spatial_ship->rotation_x != 0) {
                width /= 2;
            }
            int pos_y = start_y + y;
            Color wing_left = {105, 105, 105, 255};
            Color wing_right = {120, 120, 120, 255};
            if (spatial_ship->rotation_x < 0) {
                wing_left = (Color) {90, 90, 90, 255};
                wing_right = (Color) {105, 105, 105, 255};
            } else if (spatial_ship->rotation_x > 0) {
                wing_left = (Color) {120, 120, 120, 255};
                wing_right = (Color) {135, 135, 135, 255};
            }
            SDL_SetRenderDrawColor(renderer, wing_left.r, wing_left.g, wing_left.b, wing_left.a);
            SDL_RenderDrawLine(renderer, spatial_ship_screen_position.x - width, pos_y, spatial_ship_screen_position.x, pos_y);
            SDL_SetRenderDrawColor(renderer, wing_right.r, wing_right.g, wing_right.b, wing_right.a);
            SDL_RenderDrawLine(renderer, spatial_ship_screen_position.x, pos_y, spatial_ship_screen_position.x + width, pos_y);
        }
        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
        SDL_RenderDrawLine(renderer, spatial_ship_screen_position.x, spatial_ship_screen_position.y - (spatial_ship->size.y / 2), spatial_ship_screen_position.x, spatial_ship_screen_position.y + (spatial_ship->size.y / 2));
        // viseur du vaisseau
        SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
        int add_y = 20;
        int width = 4;
        SDL_Rect viseur = {spatial_ship_screen_position.x - (width/2), spatial_ship_screen_position.y - add_y, width, width};
        SDL_RenderFillRect(renderer, &viseur);
        // dessiner des éclair électriques aux positions aléatoire mais proche de la position du vaisseau
        // moins le vaisseau posséde de santé et plus il y a d'éclairs
        // chaque éclair est représenté par trois ligne bleu de longueur 4 pixels à la rotation aléatoire
        int nb_eclairs = 10 - ((float)(spatial_ship->health) / 10.0);
        for (int i = 0; i < nb_eclairs; i++) {
            int w = (spatial_ship->size.x * 3) / 2;
            int h = (spatial_ship->size.y * 3) / 2;
            if (spatial_ship->rotation_x != 0) {
                w = (spatial_ship->size.x * 3) / 4;
                h = (spatial_ship->size.y * 3) / 4;
            }
            int x = spatial_ship_screen_position.x + (rand() % w) - (w / 2);
            int y = spatial_ship_screen_position.y + (rand() % h) - (h / 2);
            SDL_SetRenderDrawColor(renderer, 0, 50, 220, 255);
            for (int j = 0; j < 3; j++) {
                int rotation = rand() % 360;
                int dx = cos(rotation) * 4;
                int dy = sin(rotation) * 4;
                SDL_RenderDrawLine(renderer, x, y, x + dx, y + dy);
            }
        }
    }
}

// dessiner le HUD, retourne la texture de rendu du score afin que la destruction de celle-ci ne cause pas de bug.
SDL_Texture* DrawHUD(SDL_Renderer *renderer, SpatialShip ship) {
    // dessiner la barre de vie
    SDL_Rect fond_barre_vie = {(float)(wnd_size.x) * 0.1, (float)(wnd_size.y) * 0.95, (float)(wnd_size.x) * 0.1, (float)(wnd_size.y) * 0.025};
    SDL_SetRenderDrawColor(renderer, 60, 60, 60, 255);
    SDL_RenderFillRect(renderer, &fond_barre_vie);
    SDL_Rect barre_vie = {fond_barre_vie.x, fond_barre_vie.y, (float)(fond_barre_vie.w) * (float)((float)(ship.health) / 100.0), fond_barre_vie.h};
    if (barre_vie.w < 0) {
        barre_vie.w = 0;
    }
    SDL_SetRenderDrawColor(renderer, 0, 200, 0, 255);
    SDL_RenderFillRect(renderer, &barre_vie);
    // écrire le score
    char text[50];
    sprintf(text, "Score: %d", ship.score);
    // Rendre le texte
    SDL_Surface* surface = TTF_RenderText_Solid(font, text, (SDL_Color) {255, 255, 255, 255});
    if (!surface) {
        fprintf(stderr, "Échec de rendu du texte: %s\n", TTF_GetError());
        return NULL;
    }
    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (!texture) {
        fprintf(stderr, "Échec de création de la texture: %s\n", SDL_GetError());
        return NULL;
    }
    SDL_Rect dstrect = {(float)(wnd_size.x) * 0.1, (float)(wnd_size.y) * 0.88, 0, 0};
    //SDL_Rect dstrect = { 120, 120, 0, 0 };
    SDL_QueryTexture(texture, NULL, NULL, &dstrect.w, &dstrect.h);
    SDL_RenderCopy(renderer, texture, NULL, &dstrect);
    return texture;
}

// dessiner le mini-map en haut à gauche de l'écran.
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
        if (visible_asteroid[i].health <= 0) {
            continue;
        }
        int x = ((visible_asteroid[i].position.x - ship.position.x) / 10) + centre_x;
        int y = ((visible_asteroid[i].position.y - ship.position.y) / 10) + centre_y;
        int radius = visible_asteroid[i].radius / 10;
        SDL_SetRenderDrawColor(renderer, visible_asteroid[i].color.r, visible_asteroid[i].color.g, visible_asteroid[i].color.b, visible_asteroid[i].color.a);
        for (int w = 0; w < radius * 2; w++) {
            for (int h = 0; h < radius * 2; h++) {
                int dx = radius - w;
                int dy = radius - h;
                Point p = {x + dx, y + dy};
                if (p.x < 0 || p.x >= fond.w || p.y < 0 || p.y >= fond.h) {
                    continue;
                }
                if ((dx * dx + dy * dy) <= (radius * radius)) {
                    SDL_RenderDrawPoint(renderer, p.x, p.y);
                }
            }
        }
    }
    // dessiner un ligne pour la direction du vaisseau
    SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
    SDL_RenderDrawLine(renderer, centre_x, centre_y, centre_x + (ship.angle.x*8), centre_y + (ship.angle.y*8));
    // dessiner un carré pour la position du vaisseau
    SDL_SetRenderDrawColor(renderer, 0, 255, 0, 255);
    SDL_Rect rect = {centre_x - 2, centre_y - 2, 4, 4};
    SDL_RenderFillRect(renderer, &rect);
    free(visible_asteroid);
}




// retourne les index de colonne et de ligne des blocs visibles en fonction de la position donnée d'un objet
Point* GetAdjacentBlocks(Point position) {
    int view_block_x = (position.x / wnd_size.x);
    int view_block_y = (position.y / wnd_size.y);
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
    int max_x = (game_surface.x / wnd_size.x) - 1;
    int max_y = (game_surface.y / wnd_size.y) - 1;
    for (int i = 0; i < VISIBLE_BLOCK_NUMBER; i++) {
        if (visible_blocks[i].x < 0) {
            visible_blocks[i].x = max_x; // + visible_blocks[i].x;
        } else if (visible_blocks[i].x > max_x) {
            visible_blocks[i].x = 0; //visible_blocks[i].x - max_x;
        }
        if (visible_blocks[i].y < 0) {
            visible_blocks[i].y = max_y; // + visible_blocks[i].y;
        } else if (visible_blocks[i].y > max_y) {
            visible_blocks[i].y = 0; //visible_blocks[i].y - max_y;
        }
    }
    return visible_blocks;
}

// retournr la liste des astéroides visibles
Asteroid* GetVisibleAsteroids(Asteroid*** asteroids, SpatialShip spatial_ship) {
    Point* visible_blocks = GetAdjacentBlocks(spatial_ship.position);
    Asteroid* visible_asteroids = malloc((VISIBLE_BLOCK_NUMBER * ASTEROID_PER_BLOC) * sizeof(Asteroid));
    int index = 0;
    for (int i = 0; i < VISIBLE_BLOCK_NUMBER; i++) {
        Point block = visible_blocks[i];
        Asteroid* asteroids_in_block = asteroids[(int)(block.y)][(int)(block.x)];
        for (int j = 0; j < ASTEROID_PER_BLOC; j++) {
            visible_asteroids[index] = (Asteroid) {asteroids_in_block[j].position, asteroids_in_block[j].color, asteroids_in_block[j].radius, asteroids_in_block[j].health};
            int tcheck_dist_x = (visible_asteroids[index].position.x - spatial_ship.position.x);
            if (tcheck_dist_x > wnd_size.x * 2) {
                visible_asteroids[index].position.x -= game_surface.x;
            } else if (tcheck_dist_x < -wnd_size.x * 2) {
                visible_asteroids[index].position.x += game_surface.x;
            }
            int tcheck_dist_y = (visible_asteroids[index].position.y - spatial_ship.position.y);
            if (tcheck_dist_y > wnd_size.y * 2) {
                visible_asteroids[index].position.y -= game_surface.y;
            } else if (tcheck_dist_y < -wnd_size.y * 2) {
                visible_asteroids[index].position.y += game_surface.y;
            }
            index++;
        }
    }
    free(visible_blocks);
    return visible_asteroids;
}

// retourne la position de l'objet sur l'écran
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
        screen_bullets[i] = (Bullet){bullets[i].position, bullets[i].angle, bullets[i].distance, bullets[i].speed, bullets[i].radius, bullets[i].damage, bullets[i].collisioning, bullets[i].collision};
        int tcheck_dist_x = (bullets[i].position.x - spatial_ship.position.x);
        if (abs(tcheck_dist_x) > wnd_size.x * 2) {
            if (tcheck_dist_x > 0) {
                screen_bullets[i].position.x -= game_surface.x;
            } else {
                screen_bullets[i].position.x += game_surface.x;
            }
        }
        int tcheck_dist_y = (bullets[i].position.y - spatial_ship.position.y);
        if (abs(tcheck_dist_y) > wnd_size.y * 2) {
            if (tcheck_dist_y > 0) {
                screen_bullets[i].position.y -= game_surface.y;
            } else {
                screen_bullets[i].position.y += game_surface.y;
            }
        }
        Point screen_position = CalculObjectScreenPosition(screen_bullets[i].position, spatial_ship);
        screen_bullets[i].position = screen_position;
    }
    return screen_bullets;
}
// retourne un tableau d'astéroides trié par ordre de distance par rapport au haut de l'écran, axe y
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
// retourne un tableau de balles trié par ordre de distance par rapport au haut de l'écran, axe y
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
// appelle les fonctions de gestion de la scène et dessine les objets, retourne la texture du score afin d'éviter les bugs lors de la destruction de celle-ci
SDL_Texture* Scene(SDL_Renderer *renderer, Asteroid*** asteroids, Bullet* bullets, int bullet_number, SpatialShip *ship) {
    Asteroid* visible_asteroids = GetVisibleAsteroids(asteroids, *ship);
    visible_asteroids = CalculAsteroidScreenPosition(visible_asteroids, *ship);
    visible_asteroids = SortAsteroidSceneObject(visible_asteroids);
    Bullet* screen_pos_bullets = BulletsScreenPosition(bullets, bullet_number, *ship);
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
    DrawMiniMap(renderer, asteroids, *ship);
    SDL_Texture* texture = DrawHUD(renderer, *ship);

    free(visible_asteroids);
    free(screen_pos_bullets);
    return texture;
}


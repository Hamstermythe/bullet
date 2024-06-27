#include <SDL2/SDL.h>
#include <stdbool.h>
#include <time.h>


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
} Bullet;
// structure de l'asteroide
typedef struct {
    Point position;
    int radius;
    int health;
} Asteroid;

// surface du jeu 
Point game_surface = {1000000000, 1000000000};
// taille de la fenetre
Point wnd_size = {640, 480};
// spatial ship position
Point spatial_ship_screen_position = {320, 360};
// spatial ship information
SpatialShip spatial_ship = {0, 0, 5, 100, {15, 30}, {500000000, 500000000}, {0, 1}};

#define SPATIAL_SHIP_SPEED 5
#define ASTEROID_VIEWING_DISTANCE 360
#define BULLET_COOLDOWN_MS 750
#define BULLET_LIFETIME 1200

// fonction pour retourner un nouvel objet asteroid, celui spawn dans une surface de 1000 * 1000
Point NewAsteroidPosition() {
    return (Point) {rand() % wnd_size.x, rand() % wnd_size.y};
}
// Fonction pour retourner un tableau de 100 astéroïdes
Asteroid* ChunkOfSpace() {
    int asteroid_per_bloc = 4;
    Asteroid* asteroids = malloc(asteroid_per_bloc * sizeof(Asteroid)); // Allouer de la mémoire pour 100 astéroïdes
    if (asteroids == NULL) {
        fprintf(stderr, "Allocation mémoire pour ChunkOfSpace a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < asteroid_per_bloc; i++) {
        asteroids[i].position = NewAsteroidPosition();
        asteroids[i].radius = wnd_size.x/5;
        asteroids[i].health = 100;
    }
    return asteroids;
}
// Fonction pour retourner un tableau de tableaux d'astéroïdes (matrice)
Asteroid** lineOfSpace() {
    int bloc_number = game_surface.x/wnd_size.x;
    Asteroid** line = malloc(bloc_number * sizeof(Asteroid*)); // Allouer de la mémoire pour 100 pointeurs vers des astéroïdes
    if (line == NULL) {
        fprintf(stderr, "Allocation mémoire pour lineOfSpace a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < bloc_number; i++) {
        line[i] = ChunkOfSpace();
    }
    return line;
}
// Fonction pour retourner un tableau de tableaux de tableaux d'astéroïdes (matrice)
Asteroid*** Space() {
    int lign_number = game_surface.y/wnd_size.y;
    Asteroid*** space = malloc(lign_number * sizeof(Asteroid*)); // Allouer de la mémoire pour 100 pointeurs vers des tableaux d'astéroïdes
    if (space == NULL) {
        fprintf(stderr, "Allocation mémoire pour Space a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < lign_number; i++) {
        space[i] = lineOfSpace();
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
    int radius = radius * ratio;
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
    // Conversion en coordonnées d'écran sans mise à l'échelle
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
    Bullet bullet = {ship->position, ship->angle, 0, 10, wnd_size.x*0.05, 10};
    return bullet;
}
Bullet* addBullet(Bullet* bullets, int bullet_size) {
    Bullet* newBullets = malloc(bullet_size * sizeof(Bullet));
    if (newBullets == NULL) {
        fprintf(stderr, "Allocation mémoire pour newBullets a échoué.\n");
        return NULL;
    }
    for (int i = 0; i < bullet_size; i++) {
        newBullets[i] = bullets[i];
    }
    free(bullets);
    return newBullets;
}
Bullet* removeBullet(Bullet* bullets, int *bullet_size) {
    if (bullet_size == 0) {
        return NULL;
    }
    Bullet* newBullets = malloc(len(bullets) - 1 * sizeof(Bullet));
    if (newBullets == NULL) {
        fprintf(stderr, "Allocation mémoire pour newBullets a échoué.\n");
        return NULL;
    }
    int new_bullet_size = 0;
    for (int i = 0; i < len(bullets); i++) {
        Bullet bullet = bullets[i];
        if (bullet.distance < BULLET_LIFETIME) {
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
void moveBullet(Bullet *bullet) {
    bullet->position.x += bullet->speed * bullet->angle.x;
    bullet->position.y += bullet->speed * bullet->angle.y;
    bullet->distance += bullet->speed;
}
// fonction de collision entre le bullet et l'asteroide
bool bulletAsteroidCollision(Bullet bullet, Asteroid asteroid) {
    int distance = sqrt(pow(bullet.position.x - asteroid.position.x, 2) + pow(bullet.position.y - asteroid.position.y, 2));
    return distance < bullet.radius + asteroid.radius;
}
// fonction de collision entre le vaisseau spatial et l'asteroide
bool spatialShipAsteroidCollision(SpatialShip ship, Asteroid asteroid) {
    int distance = sqrt(pow(ship.position.x - asteroid.position.x, 2) + pow(ship.position.y - asteroid.position.y, 2));
    return distance < ship.size.y + asteroid.radius;
}



// dessine un carré de 5 pixel reduit par la distance avec le vaisseau spatial
void DrawBullet(SDL_Renderer *renderer, Bullet bullet) {
    int radius = CalculateDrawRadius(bullet.position, bullet.radius);
    Point bullet_screen_position = CalculObjectScreenPosition(bullet.position);
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
    SDL_Point vertices[4] = {
            {spatial_ship_screen_position.x, spatial_ship_screen_position.y},
            {spatial_ship_screen_position.x - 10, spatial_ship_screen_position.y + 20},
            {spatial_ship_screen_position.x + 10, spatial_ship_screen_position.y + 20},
            {spatial_ship_screen_position.x, spatial_ship_screen_position.y}
    };
    SDL_SetRenderDrawColor(renderer, 80, 80, 80, 255);
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
void FreeVisibleBlocks(Point* blocks) {
    free(blocks);
}
// fonction qui retourne les object de la scène à afficher
Asteroid* GetBlocks(SDL_Renderer *renderer, Asteroid*** asteroids, SpatialShip *ship) {
    Asteroid* scene_object = malloc(16 * sizeof(Asteroid));
    int count = 0;
    Point* blocks = GetVisibleBlocks(ship);
    for (int i = 0; i < len(blocks); i++) {
        Point block = blocks[i];
        Asteroid* block_asteroids = asteroids[block.x][block.y];
        for (int j = 0; j < len(block_asteroids); j++) {
            Asteroid asteroid = block_asteroids[j];
            scene_object[count] = asteroid;
            count++;
        }
    }
    FreeVisibleBlocks(blocks);
    return scene_object;
}
void FreeSceneObject(Asteroid* scene_object) {
    free(scene_object);
}
// fonction qui renvoie un tableau d'astéroïde dont les positions sont les positions à l'écran des astéroïdes d'origine.
Asteroid* CalculSceneObjectScreenPosition(Asteroid* scene_object) {
    Asteroid* scene_object_screen_position = malloc(16 * sizeof(Asteroid));
    for (int i = 0; i < len(scene_object); i++) {
        Asteroid asteroid = scene_object[i];
        scene_object_screen_position[i].position = CalculObjectScreenPosition(asteroid.position);
        scene_object_screen_position[i].radius = asteroid.radius;
    }
    free(scene_object);
    return scene_object_screen_position;
}
// fonction qui renvoie un tableau d'astéroïde après avoir trié celles ci par ordre croissant sur la position y.
Asteroid* SortSceneObject(Asteroid* scene_object) {
    Asteroid* sorted_scene_object = malloc(16 * sizeof(Asteroid));
    int count = 0;
    for (;count < len(scene_object);) {
        for (int i = 0; i < len(scene_object); i++) {
            bool lower = true;
            int index_become_null = 0;
            Asteroid asteroid = scene_object[i];
            for (int j=0; j < len(scene_object); j++) {
                if (j == i) {
                    continue;
                }
                if (asteroid.position.y < scene_object[j].position.y) {
                    lower = false;
                    index_become_null = j;
                    break;
                }
            }
            if (lower) {
                sorted_scene_object[count] = asteroid;
                count++;
                Asteroid* scene_object_remover = malloc(16 * sizeof(Asteroid));
                for (int j = 0; j < len(scene_object); j++) {
                    if (j != index_become_null) {
                        scene_object_remover[j] = scene_object[j];
                    }
                }
                free(scene_object);
                scene_object = scene_object_remover;
            }
        }
    }
    return sorted_scene_object;
}
// fonction qui renvoie un tableau de bullet dont les positions sont les positions à l'écran des bullets d'origine.
Bullet* CalculBulletScreenPosition(Bullet* bullets) {
    Bullet* bullets_screen_position = malloc(len(bullets) * sizeof(Bullet));
    for (int i = 0; i < len(bullets); i++) {
        Bullet bullet = bullets[i];
        bullets_screen_position[i].position = CalculObjectScreenPosition(bullet.position);
    }
    return bullets_screen_position;
}
// fonction qui renvoie un tableau de bullet après avoir trié celles ci par ordre croissant sur la position y.
Bullet* SortBullet(Bullet* bullets) {
    Bullet* sorted_bullets = malloc(16 * sizeof(Bullet));
    int count = 0;
    for (;count < len(bullets);) {
        for (int i = 0; i < len(bullets); i++) {
            bool lower = true;
            int index_become_null = 0;
            Bullet bullet = bullets[i];
            for (int j=0; j < len(bullets); j++) {
                if (j == i) {
                    continue;
                }
                if (bullet.position.y < bullets[j].position.y) {
                    lower = false;
                    index_become_null = j;
                    break;
                }
            }
            if (lower) {
                sorted_bullets[count] = bullet;
                count++;
                Bullet* bullets_remover = malloc(16 * sizeof(Bullet));
                for (int j = 0; j < len(bullets); j++) {
                    if (j != index_become_null) {
                        bullets_remover[j] = bullets[j];
                    }
                }
                free(bullets);
                bullets = bullets_remover;
            }
        }
    }
    return sorted_bullets;
}


//fonction qui gère la scène
void Scene(SDL_Renderer *renderer, Asteroid*** asteroids, Bullet* bull, int bullet_number, SpatialShip *ship) {
    Asteroid* scene_object_asteroids = GeteBlocks(renderer, asteroids, ship);
    Asteroid* screen_position_asteroids = CalculSceneObjectScreenPosition(scene_object_asteroids);
    free(scene_object_asteroids);
    Asteroid* sorted_asteroids = SortSceneObject(screen_position_asteroids);
    free(screen_position_asteroids);

    Bullet* screen_position_bullets = CalculBulletScreenPosition(bull);
    Bullet* sorted_bullets = SortBullet(screen_position_bullets);
    free(screen_position_bullets);

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
    free(sorted_asteroids);
}


int main(int argc, char *argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window *window = SDL_CreateWindow("Triangle Window",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          wnd_size.x,
                                          wnd_size.y,
                                          SDL_WINDOW_SHOWN);

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    bool running = true;
    SDL_Event event;

    // temps de maintenant
    int last_bullet_shot = clock_gettime();
    // unsized empty array of bullets
    Bullet* bullets = NULL;
    int bullet_number = 0;
    // unsized empty array of asteroids
    Asteroid*** asteroids = Space();

    while (running) {


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
                if (clock_gettime() - last_bullet_shot > 1000) {
                    last_bullet_shot = clock_gettime();
                    // Shoot a bullet
                    bullet_number++;
                    bullets = addBullet(&bullets, bullet_number);
                }
            }
        }

        SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255); // Black color
        SDL_RenderClear(renderer);

        DrawSpatialShip(renderer);

        SDL_RenderPresent(renderer);
    }

    free(bullets);
    FreeSpace(asteroids);

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
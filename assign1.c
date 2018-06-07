/*
 *  Asteroids 
 *
 *  Noah Attwood
 *  B00718872
 *  CSCI 3161

 todo:
    - circular asteroids
    - draw asteroids in the correct location. When the x,y coordinate of an asteroid leaves the screen, how do we update the nVertices of the asteroid
        accordingly? Should we use some translatation? Store the radius of each vertex from the x,y point upon creation? Could do this by storing
        the information inside of the coord struct. 
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <GL/glut.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#define RAD2DEG 180.0/M_PI
#define DEG2RAD M_PI/180.0

#define myTranslate2D(x,y) glTranslated(x, y, 0.0)
#define myScale2D(x,y) glScalef(x, y, 1.0)
#define myRotate2D(angle) glRotatef(RAD2DEG*angle, 0.0, 0.0, 1.0)

#define TIME_DELTA      33

#define SPAWN_ASTEROID_PROB     0.005
#define MAX_SHIP_VELOCITY       0.05
#define PHOTON_VELOCITY         MAX_SHIP_VELOCITY * 2
#define PHOTON_LENGTH           2.5
#define MAX_PHOTONS             8
#define MAX_ASTEROIDS	        8
#define MAX_VERTICES	        16


/* -- type definitions ------------------------------------------------------ */

typedef struct Coords {
	double x, y;
} Coords;

typedef struct {
	double x, y, phi, dx, dy, radius, turnSpeed, acceleration;
} Ship;

typedef struct {
	int	active;
	double x1, y1, x2, y2, dx, dy, phi;
} Photon;

typedef struct {
	int	active, nVertices;
	double x, y, phi, dx, dy, dphi, radius;
	Coords	coords[MAX_VERTICES];
} Asteroid;


/* -- function prototypes --------------------------------------------------- */

static void	myDisplay(void);
static void	myTimer(int value);
static void myPauseTimer(int value);
static void	myKey(unsigned char key, int x, int y);
static void	keyPress(int key, int x, int y);
static void	keyRelease(int key, int x, int y);
static void	myReshape(int w, int h);

static void	init(void);
static void	initAsteroid(Asteroid *a, double x, double y, double size);
static void	drawShip(Ship *s);
static void	drawPhoton(Photon *p);
static void	drawAsteroid(Asteroid *a);

static void debug(void);
static void advanceShip(void);
static void processUserInput(void);
static void updatePhotons(void);
static void spawnAsteroids(void);
static void advanceAsteroids(void);

static double myRandom(double min, double max);
static double clamp(double value, double min, double max);
static void initPhoton(void);
static int isInBounds(double x, double y);
static void raycastForNewCoordinates(double x, double y, double dx, double dy, double * newCoords);
static void checkPhotonAsteroidCollision(void);
static void resetAsteroidShape(void);


/* -- global variables ------------------------------------------------------ */

static int	up = 0, down = 0, left = 0, right = 0, firing = 0, circularAsteroids = 1, pause = 0; // state of user input
static double xMax, yMax;
static float timer;
static Ship	ship;
static Photon	photons[MAX_PHOTONS];
static Asteroid	asteroids[MAX_ASTEROIDS];


/* -- main ------------------------------------------------------------------ */

int
main(int argc, char *argv[])
{
    srand((unsigned int) time(NULL));

    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGB);
    glutInitWindowSize(800, 600);
    glutCreateWindow("Asteroids");
    glutDisplayFunc(myDisplay);
    glutIgnoreKeyRepeat(1);
    glutKeyboardFunc(myKey);
    glutSpecialFunc(keyPress);
    glutSpecialUpFunc(keyRelease);
    glutReshapeFunc(myReshape);
    glutTimerFunc(TIME_DELTA, myTimer, 0);
    glClearColor(0.0, 0.0, 0.0, 1.0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    init();
    
    glutMainLoop();
    
    return 0;
}


/* ================================================ GLUT Callback Functions ============================================== */

void
myDisplay()
{
    /*
     *	display callback function
     */

    int	i;

    glClear(GL_COLOR_BUFFER_BIT);

    glLoadIdentity();

    drawShip(&ship);

    for (i=0; i<MAX_PHOTONS; i++)
    	if (photons[i].active){
            drawPhoton(&photons[i]);
        }

    for (i=0; i<MAX_ASTEROIDS; i++)
    	if (asteroids[i].active)
            drawAsteroid(&asteroids[i]);
    
    glutSwapBuffers();
}

void
myTimer(int value)
{
    /*
     *	timer callback function
     */

    debug();
    updatePhotons();
    processUserInput();
    advanceShip();
    spawnAsteroids();
    advanceAsteroids();
    checkPhotonAsteroidCollision();

    glutPostRedisplay();
    
    if(pause){
        glutTimerFunc(TIME_DELTA, myPauseTimer, value);
    } else {
        glutTimerFunc(TIME_DELTA, myTimer, value);
    }
}

void
myPauseTimer(int value){
    if(pause){
        glutTimerFunc(TIME_DELTA, myPauseTimer, value);
    } else {
        glutTimerFunc(TIME_DELTA, myTimer, value);
    }
    
}

void
myKey(unsigned char key, int x, int y) {
    /*
     *  keyboard callback function; add code here for firing the laser,
     *  starting and/or pausing the game, etc.
     */
    switch(key) {
        case ' ':
            initPhoton(); break;
        case 'q':
            exit(0); break;
        case 'c':
            resetAsteroidShape(); break;
        case 'p':
            pause = abs(pause - 1); break;
    }
}

void
keyPress(int key, int x, int y) {
    /*
     *  this function is called when a special key is pressed; we are
     *  interested in the cursor keys only
     */

    switch (key) {
        case 100:
            left = 1; break;
        case 101:
            up = 1; break;
        case 102:
            right = 1; break;
        case 103:
            down = 1; break;
    }
}

void
keyRelease(int key, int x, int y) {
    /*
     *  this function is called when a special key is released; we are
     *  interested in the cursor keys only
     */

    switch (key) {
        case 100:
            left = 0; break;
        case 101:
            up = 0; break;
        case 102:
            right = 0; break;
        case 103:
            down = 0; break;
    }
}

void
myReshape(int w, int h) {
    /*
     *  reshape callback function; the upper and lower boundaries of the
     *  window are at 100.0 and 0.0, respectively; the aspect ratio is
     *  determined by the aspect ratio of the viewport
     */

    xMax = 100.0*w/h;
    yMax = 100.0;

    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0.0, xMax, 0.0, yMax, -1.0, 1.0);

    glMatrixMode(GL_MODELVIEW);
}


/* ============================================ Processing Functions ======================================= */

/**
 *  Print debug information to the console
 */
void 
debug() {
    timer += TIME_DELTA;

    if (timer >= 3000) {
        printf("ship coords: (%.3f, %.3f)\n", ship.x, ship.y);
        printf("direction: %.3f\n", ship.phi);
        printf("dx: %.4f\n", ship.dx);
        printf("dy: %.4f\n\n", ship.dy);

        int i, numActiveAsteroids = 0;
        for(i = 0; i < MAX_ASTEROIDS; i++){
            if(asteroids[i].active){
                numActiveAsteroids ++;
            } 
        }

        printf("numActiveAsteroids: %d\n", numActiveAsteroids);

        /*if(numActiveAsteroids == 7){
            Asteroid ia = asteroids[inactive];
            printf("inactive asteroid stats -   x: %.3f    y: %.3f    dx: %.3f    dy: %.3f    radius: %.3f\n", ia.x, ia.y, ia.dx, ia.dy, ia.radius);
        }*/

        printf("\n\n");

        timer -= 3000;
    }
}

/**
 *  Respond to user input by turning or accelerating the ship.
 */
void 
processUserInput() {
    if (right == 1){
        ship.phi -= ship.turnSpeed;
        if (ship.phi < 0.0){
            ship.phi += 360.0;
        }
    }

    if (left == 1){
        ship.phi += ship.turnSpeed;
        if (ship.phi > 360.0){
            ship.phi -= 360.0;
        }
    }

    if (up == 1 || down == 1){
        double newDx;
        double newDy;

        if (up == 1){
            // up velocity calculation
            newDx = ship.dx - (ship.acceleration * sin((ship.phi - 90.0) * DEG2RAD) * TIME_DELTA);
            newDy = ship.dy + (ship.acceleration * cos((ship.phi - 90.0) * DEG2RAD) * TIME_DELTA);
        } else {
            // down velocity calculation. Braking acceleration is 33% slower for extra challenge
            newDx = ship.dx + ((ship.acceleration/1.5) * sin((ship.phi - 90.0) * DEG2RAD) * TIME_DELTA);
            newDy = ship.dy - ((ship.acceleration/1.5) * cos((ship.phi - 90.0) * DEG2RAD) * TIME_DELTA);
        }

        double velocityMagnitude = sqrt(pow(newDx, 2) + pow(newDy, 2));

        if (velocityMagnitude <= MAX_SHIP_VELOCITY){
            ship.dx = newDx;
            ship.dy = newDy;
        } else {
            // new velocity will exceed max ship velocity
            // subtract the difference to make it the same magnitude as the max ship velocity
            velocityMagnitude -= (velocityMagnitude - MAX_SHIP_VELOCITY);
            double thetaRadians = atan(newDy/newDx);

            if (newDx < 0){
                ship.dx = velocityMagnitude * cos(thetaRadians - M_PI);
                ship.dy = velocityMagnitude * sin(thetaRadians - M_PI);
            } else {
                ship.dx = velocityMagnitude * cos(thetaRadians);
                ship.dy = velocityMagnitude * sin(thetaRadians);
            }       
        }
    }
}

/**
 *  Advances the ship. This entails incrementing the ship's position and resetting the ship 
 *  within the bounds of the screen when it leaves.
 */
void 
advanceShip(){
    ship.x += ship.dx*TIME_DELTA;
    ship.y += ship.dy*TIME_DELTA;

    // check if ship left the screen
    if (!isInBounds(ship.x, ship.y)){
        // set the ship's new position on the screen dependent on its velocity

        // ship is out of the screen so we need to get an approximation of its coordinates within the screen
        double newCoords[2];
        raycastForNewCoordinates(ship.x, ship.y, ship.dx, ship.dy, newCoords);

        ship.x = newCoords[0];
        ship.y = newCoords[1];
    }
}

/**
 *  Iterate over all active photons and updates their position. When an active photon has left the screen, 
 *  it is changed to not active.
 */
void 
updatePhotons(){
    int i;
    for(i = 0; i < MAX_PHOTONS; i++){
        if(photons[i].active){
            Photon p = photons[i];
            p.x1 += p.dx*TIME_DELTA;
            p.y1 += p.dy*TIME_DELTA;
            p.x2 += p.dx*TIME_DELTA;
            p.y2 += p.dy*TIME_DELTA;

            if(!isInBounds(p.x1, p.y1)){
                p.active = 0;
            }

            photons[i] = p;
        }
    }
}

void
spawnAsteroids(){
    // find out if there are any active asteroids currently
    int i, numActiveAsteroids = 0;
    for (i = 0; i < MAX_ASTEROIDS; i++){
        if (asteroids[i].active){
            numActiveAsteroids++;
        }
    }

    if (numActiveAsteroids == 0 || (numActiveAsteroids < MAX_ASTEROIDS && myRandom(0.0, 1.0) < SPAWN_ASTEROID_PROB)){
        
        Asteroid newAsteroid;

        for (i = 0; i < MAX_ASTEROIDS; i++){
            if (!asteroids[i].active){
                newAsteroid = asteroids[i];
                break;
            }
        }

        // cast double return value to an integer, giving possible values in the range [0-3]
        int spawnSide = myRandom(0.0, 3.9999);

        double spawnX, spawnY;
        switch (spawnSide){
            case 0:
                // spawn on the right side of the screen
                spawnX = xMax;
                spawnY = myRandom(0.0, yMax);
                break;
            case 1:
                // spawn on the bottom side of the screen
                spawnX = myRandom(0.0, xMax);
                spawnY = 0.0;
                break;
            case 2:
                // spawn on the left side of the screen
                spawnX = 0.0;
                spawnY = myRandom(0.0, yMax);
                break;
            case 3:
                // spawn on the top side of the screen
                spawnX = myRandom(0.0, xMax);
                spawnY = yMax;
                break;
        }

        printf("Spawning an asteroid\nspawnSide: %i\nspawnX: %.3f    spawnY: %.3f\n", spawnSide, spawnX, spawnY);

        initAsteroid(&newAsteroid, spawnX, spawnY, 2.0);
        asteroids[i] = newAsteroid;
    }
}

void 
advanceAsteroids(){
    int i;
    for (i = 0; i < MAX_ASTEROIDS; i++){
        if(asteroids[i].active){
            //printf("updating asteroid at index %i\noldX: %.3f    oldY: %.3f\n", i, asteroids[i].x, asteroids[i].y);
            Asteroid a = asteroids[i];

            a.x += a.dx*TIME_DELTA;
            a.y += a.dy*TIME_DELTA;
            a.phi += a.dphi;
            //printf("newX: %.3f    newY: %.3f\n", a.x, a.y);

            if (a.x > xMax){
                a.x = 0.0;
            } else if (a.x < 0.0){
                a.x = xMax;
            }

            if (a.y > yMax){
                a.y = 0.0;
            } else if (a.y < 0.0){
                a.y = yMax;
            }

            /*int j;
            for (j = 0; j < a.nVertices; j++){
                a.coords[j].x += a.dx*TIME_DELTA;
                a.coords[j].y += a.dy*TIME_DELTA;
            }*/

            asteroids[i] = a;
        }
    }
}


/* ============================================= Drawing Functions ========================================= */

void
drawShip(Ship *s) {

    float xVertex, yVertex;
    float t1 = (0.0f+s->phi) * DEG2RAD, 
          t2 = (135.0f+s->phi) * DEG2RAD, 
          t3 = (225.0f+s->phi) * DEG2RAD; 

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_LINE_LOOP);
    xVertex = s->x + (s->radius * cos(t1));
    yVertex = s->y + (s->radius * sin(t1));
    glVertex2f(xVertex, yVertex);

    xVertex = s->x + (s->radius * cos(t2));
    yVertex = s->y + (s->radius * sin(t2));
    glVertex2f(xVertex, yVertex);

    xVertex = s->x + (s->radius * cos(t3));
    yVertex = s->y + (s->radius * sin(t3));
    glVertex2f(xVertex, yVertex);

    glEnd();
}

void
drawPhoton(Photon *p) {
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_LINES);
    glVertex2f(p->x1, p->y1);
    glVertex2f(p->x2, p->y2);
    glEnd();
}

void
drawAsteroid(Asteroid *a) {
    int i;
    glLoadIdentity();

    // move to the asteroid's x and y
    myTranslate2D(a->x, a->y);
    myRotate2D(a->phi);
    
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBegin(GL_LINE_LOOP);
    for(i = 0; i < a->nVertices; i++){
        //printf("drawing asteroid vertex    x: %.3f    y: %.3f\n", a->coords[i].x, a->coords[i].y);
        glVertex2f(a->coords[i].x, a->coords[i].y);
    }
    glEnd();
}


/* ========================================== Initialization Functions ===================================== */

void
init()
{
    /*
     * set parameters including the numbers of asteroids and photons present,
     * the maximum velocity of the ship, the velocity of the laser shots, the
     * ship's coordinates and velocity, etc.
     */

    /** initialize ship **/
    ship.x = 70.0;
    ship.y = 50.0;
    ship.dx = 0.0;
    ship.dy = 0.0;
    ship.phi = 90.0;
    ship.radius = 2.5;
    ship.acceleration = 0.00002;
    ship.turnSpeed = 3.4;
}

void
initAsteroid(
    Asteroid *a,
    double x, double y, double size)
{
    /*
     *  generate an asteroid at the given position; velocity, rotational
     *  velocity, and shape are generated randomly; size serves as a scale
     *  parameter that allows generating asteroids of different sizes; feel
     *  free to adjust the parameters according to your needs
     */

    double  theta, r;
    int     i;
        
    a->x = x;
    a->y = y;
    a->phi = 0.0;
    a->dx = myRandom(-0.02, 0.02);
    a->dy = myRandom(-0.02, 0.02);
    a->dphi = myRandom(-0.1, 0.1);
    
    a->nVertices = 6+rand()%(MAX_VERTICES-6);
    
    if(circularAsteroids){
        r = size * 2.5;
        a->radius = r;
        for (i=0; i<a->nVertices; i++) {
           theta = 2.0*M_PI*i/a->nVertices;
           a->coords[i].x = r*sin(theta);
           a->coords[i].y = r*cos(theta);
        }
    } else {
        a->radius = size * 2.0;                             // todo fix this when implementing real asteroids
        for (i=0; i<a->nVertices; i++) {
           theta = 2.0*M_PI*i/a->nVertices;
           r = size*myRandom(2.0, 3.0);
           a->coords[i].x = r*sin(theta);
           a->coords[i].y = r*cos(theta);
        }
    }

    a->active = 1;
}

void 
initPhoton(){
    int i; 
    for(i = 0; i < MAX_PHOTONS; i++){
        Photon p = photons[i];
        if (p.active == 0){
            p.active = 1;
            p.phi = ship.phi;
            p.x1 = ship.x + (ship.radius * cos(p.phi * DEG2RAD));
            p.y1 = ship.y + (ship.radius * sin(p.phi * DEG2RAD));
            p.x2 = p.x1 + (PHOTON_LENGTH * cos(p.phi * DEG2RAD));
            p.y2 = p.y1 + (PHOTON_LENGTH * sin(p.phi * DEG2RAD));
            p.dx = -PHOTON_VELOCITY * sin((p.phi - 90.0) * DEG2RAD);
            p.dy = PHOTON_VELOCITY * cos((p.phi - 90.0) * DEG2RAD);
            photons[i] = p;
            //printf("firing a photon:\nv1: (%.2f, %.2f)    v2: (%.2f, %.2f)    dx: %.2f    dy: %.2f    phi: %.2f\n\n", p.x1, p.y1, p.x2, p.y2, p.dx, p.dy, p.phi);
            break;
        }
    }
}


/* ============================================== Helper Functions ========================================= */

double
myRandom(double min, double max)
{
	/* return a random number uniformly draw from [min,max] */
	return min+(max-min)*(rand()%0x7fff)/32767.0;
}

/**
 *  clamps value between min and max
 */
double 
clamp (double value, double min, double max){
    value = value <= max ? value : max;
    value = value >= min ? value : min;
    return value;
}

/**
 *  checks to see if x and y are within the bounds of the screen
 */
int
isInBounds(double x, double y){
    if(x < 0 || x > xMax || y < 0 || y > yMax){
        return 0;
    }

    return 1;
}


void 
raycastForNewCoordinates(double x, double y, double dx, double dy, double * newCoords){
    // assume that x and y are outside of the bounds of the screen. They must be brought back into the screen
    double reEnterX = clamp(x, 0.0, xMax);
    double reEnterY = clamp(y, 0.0, yMax);

    // perform a raycast of the opposite of the object's current velocity. When the ray goes out of the screen, 
    // this gives an approximation of where the object should re-enter 
    while (isInBounds(reEnterX, reEnterY)){
        reEnterX -= dx*2;
        reEnterY -= dy*2;
    }

    // clamp the approximated values to within the screen
    newCoords[0] = clamp(reEnterX, 0.0, xMax);
    newCoords[1] = clamp(reEnterY, 0.0, yMax);
}

void 
checkPhotonAsteroidCollision(){
    int photonIndex, asteroidIndex;
    for(photonIndex = 0; photonIndex < MAX_PHOTONS; photonIndex ++){
        if (photons[photonIndex].active){
            // for each active photon...
            for (asteroidIndex = 0; asteroidIndex < MAX_ASTEROIDS; asteroidIndex ++){
                if (asteroids[asteroidIndex].active){
                    // ... and for each active asteroid, check if there was a collision  
                    Photon p = photons[photonIndex];
                    Asteroid a = asteroids[asteroidIndex];

                    double xPow1 = pow((p.x2 - a.x), 2);
                    double yPow1 = pow((p.y2 - a.y), 2);
                    double xPow2 = pow((p.x1 - a.x), 2);
                    double yPow2 = pow((p.y1 - a.y), 2);
                    double r2 = pow(a.radius, 2);

                    if(xPow1 + yPow1 <= r2 || xPow2 + yPow2 <= r2){
                        // there was a collision between this photon and this asteroid
                        photons[photonIndex].active = 0;
                        asteroids[asteroidIndex].active = 0;
                        
                        if(a.radius > 2.2){
                            // if the asteroid is big enough, create some child asteroids
                            int i, maxChildAsteroids = 3, childAsteroidCount = 0;
                            for(i = 0; i < MAX_ASTEROIDS; i++){
                                if (!asteroids[i].active && childAsteroidCount < maxChildAsteroids){
                                    childAsteroidCount++;

                                    initAsteroid(&asteroids[i], a.x, a.y, a.radius/4.0);
                                }
                            }
                        }

                        break;
                    } 
                }
            }
        }
    }
}

void 
resetAsteroidShape(){
    // change value from 0 -> 1 or from 1 -> 0 and reset asteroids
    circularAsteroids = abs(circularAsteroids - 1); 
    int i;
    for (i = 0; i < MAX_ASTEROIDS; i ++){
        asteroids[i].active = 0;
    }
}
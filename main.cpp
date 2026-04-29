#include <freeglut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <vector> 

#define SKYBOX_SIZE 1000.0f
#define PI 3.14159265f

struct BoundingBox {
    float minX, maxX;
    float minZ, maxZ;
};

std::vector<BoundingBox> buildingBoxes;
std::vector<BoundingBox> lampBoxes;

// ==================== AVION/ELICOPTER  ====================
enum AirplaneState {
    AP_PARCAT,
    AP_DECOLARE,
    AP_ZBOR,
    AP_ATERIZARE
};

struct Airplane {
    float x, y, z;
    float angle;
    float pitch;
    float speed;
    float timer;
    float bankAngle;
    float targetAngle;
    AirplaneState state;
    float stateTimer;   
};

struct Helicopter {
    float x, y, z;
    float angle;
    float targetAngle;
    float speed;
    float timer;
    float bankAngle;
    float rotorAngle;    
    float tailRotorAngle; 
    float bobY;          
};

Helicopter helicopter = { 100.0f, 60.0f, 100.0f, 0.0f, 0.0f, 1.8f, 0.0f, 0.0f, 0.0f, 0.0f };


const float RUNWAY_X = 400.0f;   
const float RUNWAY_Z = 200.0f;   
const float RUNWAY_ANGLE = 0.0f; 

Airplane airplane = { 400.0f, -9.0f, 200.0f, 0.0f, 0.0f, 2.5f, 0.0f, 0.0f };


// ==================== MASINI GIRATORIU ====================
struct CircularCar {
    float angle;
    float speed;
    float radius;
    int colorIdx;
};
CircularCar circCars[3];

bool objectsInitialized = false;

// Definirea constantei pentru compatibilitate cu versiuni vechi de OpenGL
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// Variabile pentru texturi
GLuint texFront, texBack, texLeft, texRight, texTop, texBottom, texGrass, texStone, texRoad, texBuilding, texTree, texLake, texShadow, texLamp, texCar;

// Variabile pentru camera (Pozitie si Rotatie)
float camX = 0.0f, camY = 3.0f, camZ = 150.0f;
float camAngleX = 0.0f;
float camAngleY = 0.0f;

// Variabile pentru mouse
int lastX, lastY;
bool mouseDown = false;

// --- LOGICA VEHICUL ---
bool isInCar = false;
bool keys[256];

float carX = 0.0f, carZ = 160.0f;
float carAngle = 0.0f, carSpeed = 0.0f;
const float CAR_ACCEL = 0.04f, CAR_BRAKE = 0.08f, CAR_FRICTION = 0.015f, CAR_MAX_SPEED = 2.5f;

float distCamera = 40.0f;
float inaltimeCamera = 12.0f;

/* ==================== INCARCARE TEXTURI ==================== */
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GLuint LoadTexture(const char* filename, bool wrap) {
    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    // Setam modul de wrap: CLAMP pentru skybox (fara margini vizibile) sau REPEAT pentru iarba
    GLint wrapMode = wrap ? GL_REPEAT : GL_CLAMP_TO_EDGE;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, wrapMode);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load(filename, &width, &height, &nrChannels, 0);

    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        gluBuild2DMipmaps(GL_TEXTURE_2D, format, width, height, format, GL_UNSIGNED_BYTE, data);
    }
    else {
        printf("Eroare la incarcarea: %s | Motiv: %s\n", filename, stbi_failure_reason());
    }

    stbi_image_free(data);
    return texture;
}

/* ==================== SISTEM DE LUMINA ==================== */
void SetupLighting() {
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    GLfloat lightPos[] = { 150.0f, 400.0f, 150.0f, 1.0f };
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    GLfloat ambient[] = { 0.15f, 0.15f, 0.2f, 1.0f };

    GLfloat diffuse[] = { 0.3f, 0.3f, 0.35f, 1.0f };

    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, diffuse);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);
}

/* ==================== DESENARE SCENA ==================== */

void DrawSkybox() {
    float s = SKYBOX_SIZE / 2.0f;
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);

    // FATA
    glBindTexture(GL_TEXTURE_2D, texFront);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(s, s, s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, s);
    glEnd();

    // SPATE
    glBindTexture(GL_TEXTURE_2D, texBack);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(-s, s, -s);
    glTexCoord2f(0, 1); glVertex3f(s, s, -s);
    glEnd();

    // STÂNGA
    glBindTexture(GL_TEXTURE_2D, texLeft);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(-s, -s, s);
    glTexCoord2f(1, 1); glVertex3f(-s, s, s);
    glTexCoord2f(0, 1); glVertex3f(-s, s, -s);
    glEnd();

    // DREAPTA
    glBindTexture(GL_TEXTURE_2D, texRight);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(s, -s, s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, s, -s);
    glTexCoord2f(0, 1); glVertex3f(s, s, s);
    glEnd();

    // TAVAN
    glBindTexture(GL_TEXTURE_2D, texTop);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1); glVertex3f(-s, s, -s);
    glTexCoord2f(0, 0); glVertex3f(-s, s, s);
    glTexCoord2f(1, 0); glVertex3f(s, s, s);
    glTexCoord2f(1, 1); glVertex3f(s, s, -s);
    glEnd();

    // PODEA CUB (Sub relief)
    glBindTexture(GL_TEXTURE_2D, texBottom);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-s, -s, -s);
    glTexCoord2f(1, 0); glVertex3f(s, -s, -s);
    glTexCoord2f(1, 1); glVertex3f(s, -s, s);
    glTexCoord2f(0, 1); glVertex3f(-s, -s, s);
    glEnd();
}

float getTerrainHeight(float x, float z) {
    float dist = sqrt(x * x + z * z);
    float h = (float)(cos(dist * 0.02f) * exp(-dist * 0.015f) * 30.0f);
    return (dist > 180.0f) ? 0.0f : h;
}

void DrawRelief() {
    glEnable(GL_TEXTURE_2D);
    float d_densitate = 15.0f;

    glBindTexture(GL_TEXTURE_2D, texGrass);
    glBegin(GL_QUADS);
    for (float x = -1000.0f; x < 1000.0f; x += 10.0f) {
        for (float z = -1000.0f; z < 1000.0f; z += 10.0f) {
            float h = getTerrainHeight(x, z);
            float dist = sqrt(x * x + z * z);

            if (h <= 2.5f || dist > 110.0f) {
                glTexCoord2f(x / d_densitate, z / d_densitate);
                glVertex3f(x, h - 10.0f, z);
                glTexCoord2f((x + 10) / d_densitate, z / d_densitate);
                glVertex3f(x + 10, getTerrainHeight(x + 10, z) - 10.0f, z);
                glTexCoord2f((x + 10) / d_densitate, (z + 10) / d_densitate);
                glVertex3f(x + 10, getTerrainHeight(x + 10, z + 10) - 10.0f, z + 10);
                glTexCoord2f(x / d_densitate, (z + 10) / d_densitate);
                glVertex3f(x, getTerrainHeight(x, z + 10) - 10.0f, z + 10);
            }

            float distLac = sqrt(pow(x - 300.0f, 2) + pow(z - 300.0f, 2));
            if (distLac < 80.0f) continue;
        }
    }
    glEnd();

    glBindTexture(GL_TEXTURE_2D, texStone);
    glBegin(GL_QUADS);
    for (float x = -400.0f; x < 400.0f; x += 10.0f) {
        for (float z = -400.0f; z < 400.0f; z += 10.0f) {
            float h = getTerrainHeight(x, z);
            float dist = sqrt(x * x + z * z);

            if (h > 2.5f && dist <= 110.0f) {
                glTexCoord2f(x / d_densitate, z / d_densitate);
                glVertex3f(x, h - 10.0f, z);
                glTexCoord2f((x + 10) / d_densitate, z / d_densitate);
                glVertex3f(x + 10, getTerrainHeight(x + 10, z) - 10.0f, z);
                glTexCoord2f((x + 10) / d_densitate, (z + 10) / d_densitate);
                glVertex3f(x + 10, getTerrainHeight(x + 10, z + 10) - 10.0f, z + 10);
                glTexCoord2f(x / d_densitate, (z + 10) / d_densitate);
                glVertex3f(x, getTerrainHeight(x, z + 10) - 10.0f, z + 10);
            }
        }
    }
    glEnd();
}

void DrawRunway() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texRoad);
    glColor3f(0.6f, 0.6f, 0.6f);

    glPushMatrix();
    glTranslatef(RUNWAY_X, -9.9f, RUNWAY_Z);
    glRotatef(RUNWAY_ANGLE, 0.0f, 1.0f, 0.0f);

    float lungime = 200.0f;
    float latime = 20.0f;
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex3f(-latime / 2, 0, -lungime / 2);
    glTexCoord2f(1, 0); glVertex3f(latime / 2, 0, -lungime / 2);
    glTexCoord2f(1, 8); glVertex3f(latime / 2, 0, lungime / 2);
    glTexCoord2f(0, 8); glVertex3f(-latime / 2, 0, lungime / 2);
    glEnd();

    // Marcaje centrale
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
    for (float lz = -lungime / 2 + 10.0f; lz < lungime / 2 - 10.0f; lz += 20.0f) {
        glBegin(GL_QUADS);
        glVertex3f(-0.8f, 0.01f, lz);
        glVertex3f(0.8f, 0.01f, lz);
        glVertex3f(0.8f, 0.01f, lz + 10.0f);
        glVertex3f(-0.8f, 0.01f, lz + 10.0f);
        glEnd();
    }

    // Marcaj START 
    glColor3f(1.0f, 1.0f, 1.0f);
    for (float lx = -latime / 2 + 1.0f; lx < latime / 2; lx += 3.5f) {
        glBegin(GL_QUADS);
        glVertex3f(lx, 0.01f, -lungime / 2);
        glVertex3f(lx + 2.0f, 0.01f, -lungime / 2);
        glVertex3f(lx + 2.0f, 0.01f, -lungime / 2 + 12.0f);
        glVertex3f(lx, 0.01f, -lungime / 2 + 12.0f);
        glEnd();
    }

    // Marcaj STOP
    for (float lx = -latime / 2 + 1.0f; lx < latime / 2; lx += 3.5f) {
        glBegin(GL_QUADS);
        glVertex3f(lx, 0.01f, lungime / 2 - 12.0f);
        glVertex3f(lx + 2.0f, 0.01f, lungime / 2 - 12.0f);
        glVertex3f(lx + 2.0f, 0.01f, lungime / 2);
        glVertex3f(lx, 0.01f, lungime / 2);
        glEnd();
    }

    // Lumini 
    glDisable(GL_LIGHTING);
    for (float lz = -lungime / 2; lz <= lungime / 2; lz += 20.0f) {
        // Stanga - rosu
        glColor3f(1.0f, 0.0f, 0.0f);
        glPushMatrix();
        glTranslatef(-latime / 2 - 1.0f, 0.3f, lz);
        glutSolidSphere(0.4f, 6, 6);
        glPopMatrix();

        // Dreapta - verde
        glColor3f(0.0f, 1.0f, 0.0f);
        glPushMatrix();
        glTranslatef(latime / 2 + 1.0f, 0.3f, lz);
        glutSolidSphere(0.4f, 6, 6);
        glPopMatrix();
    }

    // Lumini de capat (albe - threshold lights)
    glColor3f(1.0f, 1.0f, 0.8f);
    for (float lx = -latime / 2; lx <= latime / 2; lx += 4.0f) {
        glPushMatrix();
        glTranslatef(lx, 0.3f, -lungime / 2);
        glutSolidSphere(0.5f, 6, 6);
        glPopMatrix();

        glPushMatrix();
        glTranslatef(lx, 0.3f, lungime / 2);
        glutSolidSphere(0.5f, 6, 6);
        glPopMatrix();
    }
    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}


void DrawCircuit() {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texRoad);
    glColor3f(1.0f, 1.0f, 1.0f);

    float latimeDrum = 20.0f;
    float razaGiratoriu = 135.0f;

    // Sensul giratoriu
    glBegin(GL_QUAD_STRIP);
    for (int i = 0; i <= 360; i += 5) {
        float unghi = i * PI / 180.0f;
        float x = cos(unghi);
        float z = sin(unghi);

        glTexCoord2f(0, i / 10.0f);
        glVertex3f(x * (razaGiratoriu - latimeDrum), -9.9f, z * (razaGiratoriu - latimeDrum));
        glTexCoord2f(1, i / 10.0f);
        glVertex3f(x * (razaGiratoriu + latimeDrum), -9.9f, z * (razaGiratoriu + latimeDrum));
    }
    glEnd();


    float lungimeDrum = 800.0f;

    for (int j = 0; j < 4; j++) {
        float unghiIesire = j * 90.0f * PI / 180.0f;
        float dx = cos(unghiIesire);
        float dz = sin(unghiIesire);

        // Calculam vectorul perpendicular pentru latime
        float px = -dz;
        float pz = dx;

        glBegin(GL_QUADS);
        // Start linie dreapta
        float startDist = razaGiratoriu;
        float endDist = startDist + lungimeDrum;

        glTexCoord2f(0, 0);
        glVertex3f(dx * startDist - px * latimeDrum, -9.9f, dz * startDist - pz * latimeDrum);
        glTexCoord2f(1, 0);
        glVertex3f(dx * startDist + px * latimeDrum, -9.9f, dz * startDist + pz * latimeDrum);
        glTexCoord2f(1, lungimeDrum / 20.0f);
        glVertex3f(dx * endDist + px * latimeDrum, -9.9f, dz * endDist + pz * latimeDrum);
        glTexCoord2f(0, lungimeDrum / 20.0f);
        glVertex3f(dx * endDist - px * latimeDrum, -9.9f, dz * endDist - pz * latimeDrum);
        glEnd();
    }
}


void DrawRoadMarkings() {
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);

    float lungimeLinie = 10.0f;
    float spatiuIntre = 20.0f;
    float latimeLinie = 1.0f;
    float inaltimeMarkaj = -9.85f;
    // Desenam marcaje pe cele 4 directii 
    for (float d = 160.0f; d < 920.0f; d += (lungimeLinie + spatiuIntre)) {
        glBegin(GL_QUADS);

        glVertex3f(-latimeLinie, inaltimeMarkaj, d);
        glVertex3f(latimeLinie, inaltimeMarkaj, d);
        glVertex3f(latimeLinie, inaltimeMarkaj, d + lungimeLinie);
        glVertex3f(-latimeLinie, inaltimeMarkaj, d + lungimeLinie);

        glVertex3f(-latimeLinie, inaltimeMarkaj, -d);
        glVertex3f(latimeLinie, inaltimeMarkaj, -d);
        glVertex3f(latimeLinie, inaltimeMarkaj, -d - lungimeLinie);
        glVertex3f(-latimeLinie, inaltimeMarkaj, -d - lungimeLinie);

        glVertex3f(d, inaltimeMarkaj, -latimeLinie);
        glVertex3f(d + lungimeLinie, inaltimeMarkaj, -latimeLinie);
        glVertex3f(d + lungimeLinie, inaltimeMarkaj, latimeLinie);
        glVertex3f(d, inaltimeMarkaj, latimeLinie);

        glVertex3f(-d, inaltimeMarkaj, -latimeLinie);
        glVertex3f(-d - lungimeLinie, inaltimeMarkaj, -latimeLinie);
        glVertex3f(-d - lungimeLinie, inaltimeMarkaj, latimeLinie);
        glVertex3f(-d, inaltimeMarkaj, latimeLinie);
        glEnd();
    }
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
}

void DrawBuilding(float x, float z, float w, float h, float d, bool saveBox) {


    if (saveBox) {
        BoundingBox box;
        box.minX = x - w / 2.0f;
        box.maxX = x + w / 2.0f;
        box.minZ = z - d / 2.0f;
        box.maxZ = z + d / 2.0f;
        buildingBoxes.push_back(box);
    }

    glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
    glEnable(GL_BLEND); glColor4f(0.0f, 0.0f, 0.0f, 0.4f);
    glBegin(GL_QUADS);
    glVertex3f(x - w / 1.8f + 8.0f, -9.96f, z + d / 1.8f + 8.0f);
    glVertex3f(x + w / 1.8f + 25.0f, -9.96f, z + d / 1.8f + 8.0f);
    glVertex3f(x + w / 1.8f + 25.0f, -9.96f, z - d / 1.8f + 8.0f);
    glVertex3f(x - w / 1.8f + 8.0f, -9.96f, z - d / 1.8f + 8.0f);
    glEnd();
    glEnable(GL_LIGHTING); glEnable(GL_TEXTURE_2D);


    glBindTexture(GL_TEXTURE_2D, texBuilding);
    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glTexCoord2f(0, 0); glVertex3f(x - w / 2, -10, z + d / 2);
    glTexCoord2f(w / 20, 0); glVertex3f(x + w / 2, -10, z + d / 2);
    glTexCoord2f(w / 20, h / 20); glVertex3f(x + w / 2, -10 + h, z + d / 2);
    glTexCoord2f(0, h / 20); glVertex3f(x - w / 2, -10 + h, z + d / 2);
    glBindTexture(GL_TEXTURE_2D, texBuilding);

    float repeatW = w / 20.0f;
    float repeatH = h / 20.0f;
    float repeatD = d / 20.0f;

    glBegin(GL_QUADS);
    // FATA
    glTexCoord2f(0, 0);             glVertex3f(x - w / 2, -10.0f, z + d / 2);
    glTexCoord2f(repeatW, 0);       glVertex3f(x + w / 2, -10.0f, z + d / 2);
    glTexCoord2f(repeatW, repeatH); glVertex3f(x + w / 2, -10.0f + h, z + d / 2);
    glTexCoord2f(0, repeatH);       glVertex3f(x - w / 2, -10.0f + h, z + d / 2);

    // SPATE
    glTexCoord2f(0, 0);             glVertex3f(x + w / 2, -10.0f, z - d / 2);
    glTexCoord2f(repeatW, 0);       glVertex3f(x - w / 2, -10.0f, z - d / 2);
    glTexCoord2f(repeatW, repeatH); glVertex3f(x - w / 2, -10.0f + h, z - d / 2);
    glTexCoord2f(0, repeatH);       glVertex3f(x + w / 2, -10.0f + h, z - d / 2);

    // STANGA
    glTexCoord2f(0, 0);             glVertex3f(x - w / 2, -10.0f, z - d / 2);
    glTexCoord2f(repeatD, 0);       glVertex3f(x - w / 2, -10.0f, z + d / 2);
    glTexCoord2f(repeatD, repeatH); glVertex3f(x - w / 2, -10.0f + h, z + d / 2);
    glTexCoord2f(0, repeatH);       glVertex3f(x - w / 2, -10.0f + h, z - d / 2);

    // DREAPTA
    glTexCoord2f(0, 0);             glVertex3f(x + w / 2, -10.0f, z + d / 2);
    glTexCoord2f(repeatD, 0);       glVertex3f(x + w / 2, -10.0f, z - d / 2);
    glTexCoord2f(repeatD, repeatH); glVertex3f(x + w / 2, -10.0f + h, z - d / 2);
    glTexCoord2f(0, repeatH);       glVertex3f(x + w / 2, -10.0f + h, z + d / 2);

    // ACOPERIT
    glTexCoord2f(0, 0); glVertex3f(x - w / 2, -10.0f + h, z + d / 2);
    glTexCoord2f(2, 0); glVertex3f(x + w / 2, -10.0f + h, z + d / 2);
    glTexCoord2f(2, 2); glVertex3f(x + w / 2, -10.0f + h, z - d / 2);
    glTexCoord2f(0, 2); glVertex3f(x - w / 2, -10.0f + h, z - d / 2);
    glEnd();

    //Pentru a reda umbre pentru cladiri
    glDisable(GL_TEXTURE_2D);
    glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
    glVertex3f(x - w / 1.8, -9.95f, z + d / 1.8);
    glVertex3f(x + w / 1.8, -9.95f, z + d / 1.8);
    glVertex3f(x + w / 1.8, -9.95f, z - d / 1.8);
    glVertex3f(x - w / 1.8, -9.95f, z - d / 1.8);
    glEnd();
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
}

void DrawTree(float x, float z, float w, float h) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texTree);

    glBegin(GL_QUADS);
    // Planul 1
    glTexCoord2f(0, 0); glVertex3f(x - w / 2, -10.0f, z);
    glTexCoord2f(1, 0); glVertex3f(x + w / 2, -10.0f, z);
    glTexCoord2f(1, 1); glVertex3f(x + w / 2, -10.0f + h, z);
    glTexCoord2f(0, 1); glVertex3f(x - w / 2, -10.0f + h, z);

    // Planul 2 
    glTexCoord2f(0, 0); glVertex3f(x, -10.0f, z - w / 2);
    glTexCoord2f(1, 0); glVertex3f(x, -10.0f, z + w / 2);
    glTexCoord2f(1, 1); glVertex3f(x, -10.0f + h, z + w / 2);
    glTexCoord2f(0, 1); glVertex3f(x, -10.0f + h, z - w / 2);
    glEnd();

    glDisable(GL_BLEND);
}

void DrawLake(float centerX, float centerZ, float radius) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    float timp = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    float valuri = sin(timp * 2.0f) * 0.1f;

    glBindTexture(GL_TEXTURE_2D, texLake);
    glColor4f(0.0f, 0.5f, 0.9f, 0.6f); // Albastru cu transparenta 60%

    glBegin(GL_POLYGON);
    for (int i = 0; i < 50; i++) {
        float unghi = i * (2.0f * PI / 50.0f);
        glTexCoord2f(0.5f + 0.5f * cos(unghi), 0.5f + 0.5f * sin(unghi));
        glVertex3f(centerX + cos(unghi) * radius, -9.8f + valuri, centerZ + sin(unghi) * radius);
    }
    glEnd();

    glDisable(GL_BLEND);
    glColor3f(1.0f, 1.0f, 1.0f);
}

void DrawLightCone(float x, float y, float z, float rotY) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(rotY, 0, 1, 0);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glBegin(GL_TRIANGLE_FAN);

    glColor4f(1.0f, 1.0f, 0.7f, 0.3f);
    glVertex3f(0, 0, 0);


    for (int i = 0; i <= 30; i++) {
        float angle = i * 2.0f * PI / 30.0f;
        glColor4f(1.0f, 1.0f, 0.7f, 0.0f);
        glVertex3f(cos(angle) * 10.0f, -30.0f, sin(angle) * 10.0f);
    }
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

void DrawSoftLight(float x, float y, float z, float r) {
    glPushMatrix();
    glTranslatef(x, y, z);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);

    glBindTexture(GL_TEXTURE_2D, texShadow);

    glColor4f(1.0f, 1.0f, 0.7f, 0.4f);

    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glTexCoord2f(0, 0); glVertex3f(-r, -9.98f, -r);
    glTexCoord2f(1, 0); glVertex3f(r, -9.98f, -r);
    glTexCoord2f(1, 1); glVertex3f(r, -9.98f, r);
    glTexCoord2f(0, 1); glVertex3f(-r, -9.98f, r);
    glEnd();

    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
    glPopMatrix();
}


void DrawGroundProjection(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);

    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_TEXTURE_2D);

    glBegin(GL_TRIANGLE_FAN);

    glColor4f(1.0f, 1.0f, 0.8f, 0.7f);
    glVertex3f(0.0f, 0.03f, 0.0f);

    for (int i = 0; i <= 36; i++) {
        float angle = i * 10.0f * PI / 180.0f;

        float noise = 1.0f +
            0.3f * sin(angle * 4.0f) +
            0.15f * cos(angle * 8.0f) +
            0.1f * sin(angle * 2.0f);

        float radius = 8.0f * noise;
        glColor4f(1.0f, 1.0f, 0.8f, 0.0f);
        glVertex3f(cos(angle) * radius, 0.0f, sin(angle) * radius);
    }
    glEnd();

    glEnable(GL_LIGHTING);
    glDisable(GL_BLEND);
    glPopMatrix();
}


void DrawLampPost(float x, float z, float facingAngle, bool transparentPass) {

    if (!transparentPass) {
        BoundingBox box;
        box.minX = x - 1.5f;
        box.maxX = x + 1.5f;
        box.minZ = z - 1.5f;
        box.maxZ = z + 1.5f;
        lampBoxes.push_back(box);
    }

    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(facingAngle, 0.0f, 1.0f, 0.0f);

    if (!transparentPass) {

        glEnable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texLamp);
        glColor3f(1.0f, 1.0f, 1.0f);

        glBegin(GL_QUADS);

        glNormal3f(0.0f, 0.0f, 1.0f);
        glTexCoord2f(0, 0); glVertex3f(-0.5f, -10.0f, 0.5f);
        glTexCoord2f(1, 0); glVertex3f(0.5f, -10.0f, 0.5f);
        glTexCoord2f(1, 5); glVertex3f(0.5f, 20.0f, 0.5f);
        glTexCoord2f(0, 5); glVertex3f(-0.5f, 20.0f, 0.5f);

        glNormal3f(0.0f, 0.0f, -1.0f);
        glTexCoord2f(0, 0); glVertex3f(0.5f, -10.0f, -0.5f);
        glTexCoord2f(1, 0); glVertex3f(-0.5f, -10.0f, -0.5f);
        glTexCoord2f(1, 5); glVertex3f(-0.5f, 20.0f, -0.5f);
        glTexCoord2f(0, 5); glVertex3f(0.5f, 20.0f, -0.5f);

        glNormal3f(-1.0f, 0.0f, 0.0f);
        glTexCoord2f(0, 0); glVertex3f(-0.5f, -10.0f, -0.5f);
        glTexCoord2f(1, 0); glVertex3f(-0.5f, -10.0f, 0.5f);
        glTexCoord2f(1, 5); glVertex3f(-0.5f, 20.0f, 0.5f);
        glTexCoord2f(0, 5); glVertex3f(-0.5f, 20.0f, -0.5f);

        glNormal3f(1.0f, 0.0f, 0.0f);
        glTexCoord2f(0, 0); glVertex3f(0.5f, -10.0f, 0.5f);
        glTexCoord2f(1, 0); glVertex3f(0.5f, -10.0f, -0.5f);
        glTexCoord2f(1, 5); glVertex3f(0.5f, 20.0f, -0.5f);
        glTexCoord2f(0, 5); glVertex3f(0.5f, 20.0f, 0.5f);
        glEnd();

        glDisable(GL_LIGHTING); glDisable(GL_TEXTURE_2D);
        glColor3f(0.0f, 0.5f, 1.0f); glLineWidth(3.0f);
        glBegin(GL_LINES);
        glVertex3f(-0.51f, -10.0f, 0.51f); glVertex3f(-0.51f, 20.0f, 0.51f);
        glVertex3f(0.51f, -10.0f, 0.51f); glVertex3f(0.51f, 20.0f, 0.51f);
        glEnd();

        glColor3f(0.2f, 0.2f, 0.2f);
        glBegin(GL_QUADS);
        glVertex3f(-0.6f, 20.0f, -0.6f); glVertex3f(0.6f, 20.0f, -0.6f);
        glVertex3f(4.6f, 23.0f, 0.6f); glVertex3f(3.4f, 23.0f, -0.6f);
        glEnd();
        glColor3f(1.0f, 1.0f, 1.0f);
        glBegin(GL_QUADS);
        glVertex3f(0.6f, 19.9f, -0.5f); glVertex3f(0.6f, 19.9f, 0.5f);
        glVertex3f(4.5f, 22.8f, 0.5f); glVertex3f(4.5f, 22.8f, -0.5f);
        glEnd();
        glEnable(GL_LIGHTING);

    }
    else {

        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        //Umbra proiectata de soare
        glColor4f(0.0f, 0.0f, 0.0f, 0.3f);
        glBegin(GL_QUADS);
        glVertex3f(-0.5f, -9.99f, 0.5f);
        glVertex3f(0.5f, -9.99f, 0.5f);
        glVertex3f(15.0f, -9.99f, 35.0f);
        glVertex3f(14.0f, -9.99f, 35.0f);
        glEnd();

        // Umbre multiple soft
        float shadows[3][4] = { {0.0f, 0.0f, 15.0f, 0.35f}, {6.0f, 4.0f, 12.0f, 0.2f}, {-5.0f, 8.0f, 10.0f, 0.15f} };
        for (int s = 0; s < 3; s++) {
            float offX = shadows[s][0], offZ = shadows[s][1], r = shadows[s][2], alpha = shadows[s][3];
            glBegin(GL_TRIANGLE_FAN);
            glColor4f(0.0f, 0.0f, 0.0f, alpha);
            glVertex3f(offX, -9.98f, offZ);
            for (int i = 0; i <= 40; i++) {
                float angle = i * 2.0f * PI / 40.0f;
                glColor4f(0.0f, 0.0f, 0.0f, 0.0f);
                glVertex3f(offX + cos(angle) * r, -9.98f, offZ + sin(angle) * r);
            }
            glEnd();
        }

        glBegin(GL_TRIANGLE_FAN);
        glColor4f(1.0f, 1.0f, 0.8f, 0.3f);
        glVertex3f(4.0f, -9.97f, 0.0f);

        for (int i = 0; i <= 30; i++) {
            float angle = i * 2.0f * PI / 30.0f;
            float radius = 10.0f;
            glColor4f(1.0f, 1.0f, 0.8f, 0.0f);
            glVertex3f(4.0f + cos(angle) * radius, -9.97f, sin(angle) * radius);
        }
        glEnd();

        DrawLightCone(4.0f, 22.8f, 0.0f, 0.0f);

        glDisable(GL_BLEND);
    }
    glPopMatrix();
}


void DrawBench(float x, float z, float facingAngle) {
    glPushMatrix();
    glTranslatef(x, 0.0f, z);
    glRotatef(facingAngle, 0.0f, 1.0f, 0.0f);
    glColor3f(0.5f, 0.35f, 0.05f);

    // Sezut 
    glPushMatrix();
    glTranslatef(0.0f, -8.0f, 0.0f);
    glScalef(10.0f, 0.8f, 3.0f);
    glutSolidCube(1.0f);
    glPopMatrix();

    // Spatar
    glPushMatrix();
    glTranslatef(0.0f, -6.5f, -1.5f);
    glScalef(10.0f, 3.0f, 0.5f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glColor3f(0.1f, 0.1f, 0.1f);
    float legW = 0.3f, legH = 2.0f;
    float lX = 4.5f, lZ = 1.2f;
    float positions[4][2] = { {-lX, lZ}, {lX, lZ}, {-lX, -lZ}, {lX, -lZ} };
    for (int i = 0; i < 4; i++) {
        glPushMatrix();
        glTranslatef(positions[i][0], -9.0f, positions[i][1]); 
        glScalef(legW, legH, legW);
        glutSolidCube(1.0f);
        glPopMatrix();
    }
    glPopMatrix();
}

void DrawBenchShadow(float bX, float bZ, float lX, float lZ) {
    float dx = bX - lX;
    float dz = bZ - lZ;
    float dist = sqrt(dx * dx + dz * dz);

    if (dist < 18.0f) {
        glDisable(GL_LIGHTING);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        float maxAlpha = 0.75f * (1.0f - (dist / 18.0f));
        if (maxAlpha < 0.0f) maxAlpha = 0.0f;

        float dirX = dx / dist;
        float dirZ = dz / dist;
        float sLen = 5.0f;

        glBegin(GL_QUADS);

        glColor4f(0.0f, 0.0f, 0.0f, maxAlpha);
        glVertex3f(bX - 6.0f, -9.97f, bZ - 1.5f);
        glVertex3f(bX + 6.0f, -9.97f, bZ - 1.5f);

        //Varful umbrei
        glColor4f(0.0f, 0.0f, 0.0f, maxAlpha * 0.4f);
        glVertex3f(bX + dirX * sLen + 7.5f, -9.97f, bZ + dirZ * sLen + 2.0f);
        glVertex3f(bX + dirX * sLen - 7.5f, -9.97f, bZ + dirZ * sLen + 2.0f);
        glEnd();

        glDisable(GL_BLEND);
        glEnable(GL_LIGHTING);
    }
}

void DrawEnvironment() {
    srand(42);

    buildingBoxes.clear();
    lampBoxes.clear();

    for (int i = 0; i < 30; i++) {
        float x, z;
        bool pozitieValida = false;
        int incercari = 0;

        while (!pozitieValida && incercari < 100) {
            x = (float)(rand() % 1600 - 800);
            z = (float)(rand() % 1600 - 800);
            float distCentru = sqrt(x * x + z * z);
            pozitieValida = true;
            incercari++;

            if (distCentru < 165.0f) pozitieValida = false;
            if (fabsf(x) < 45.0f && fabsf(z) > 130.0f) pozitieValida = false;
            if (fabsf(z) < 45.0f && fabsf(x) > 130.0f) pozitieValida = false;
            if (sqrt(pow(x - 300, 2) + pow(z - 300, 2)) < 110.0f) pozitieValida = false;
            if (fabsf(x - 400.0f) < 80.0f && fabsf(z - 200.0f) < 150.0f)
                pozitieValida = false;
        }

        if (pozitieValida) {

            float w = 40.0f + (rand() % 20);
            float h = 80.0f + (rand() % 120);
            float d = 40.0f + (rand() % 20);

            DrawBuilding(x, z, w, h, d, true);
        }
    }

    for (int i = 0; i < 150; i++) {
        float tx = (float)(rand() % 1800 - 900);
        float tz = (float)(rand() % 1800 - 900);
        float distC = sqrt(tx * tx + tz * tz);

        if (distC > 160.0f && !(fabsf(tx) < 35.0f) && !(fabsf(tz) < 35.0f)) {
            DrawTree(tx, tz, 15.0f, 20.0f + (rand() % 10));
        }
    }

    float benchX = 29.5f;
    float benchZ = 165.0f;
    DrawBench(benchX, benchZ, -90.0f);
    DrawBenchShadow(benchX, benchZ, 29.0f, 160.0f);

    for (float d = 160.0f; d < 800.0f; d += 100.0f) {
        DrawLampPost(35.0f, d, 180.0f, false);
        DrawLampPost(-35.0f, -d, 0.0f, false);
        DrawLampPost(d, 35.0f, 90.0f, false);
        DrawLampPost(-d, -35.0f, 270.0f, false);
    }

    for (float d = 160.0f; d < 800.0f; d += 100.0f) {
        DrawLampPost(35.0f, d, 180.0f, true);
        DrawLampPost(-35.0f, -d, 0.0f, true);
        DrawLampPost(d, 35.0f, 90.0f, true);
        DrawLampPost(-d, -35.0f, 270.0f, true);
    }
}


void InitMovingObjects() {
    // --- AVION ---
    airplane.x = 400.0f;
    airplane.y = -9.0f;
    airplane.z = 200.0f;
    airplane.angle = 0.0f;
    airplane.targetAngle = 0.0f;
    airplane.state = AP_PARCAT;
    airplane.stateTimer = 3.0f;

    // --- MASINI GIRATORIU ---

    circCars[0].angle = 0.0f;
    circCars[0].speed = 1.2f;
    circCars[0].radius = 135.0f;
    circCars[0].colorIdx = 0;   // Rosu

    circCars[1].angle = 180.0f;
    circCars[1].speed = 0.9f;
    circCars[1].radius = 135.0f;
    circCars[1].colorIdx = 2;   // Albastru

	circCars[2].angle = 0.0f;
	circCars[2].speed = 1.0f;
	circCars[2].radius = 135.0f;
	circCars[2].colorIdx = 3;   // Verde
}



void DrawWheel(float x, float y, float z) {
    glPushMatrix();
    glTranslatef(x, y, z);

    glRotatef(90.0f, 0.0f, 1.0f, 0.0f);

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.1f, 0.1f, 0.1f);

    // glutSolidTorus(raza_tub, raza_totala, segmente_tub, segmente_roata)
    glutSolidTorus(0.4, 0.8, 15, 15);

    glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}

void DrawHelicopter(float x, float y, float z, float angle, float bankAngle, float rotorAngle, float tailRotorAngle) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(-angle, 0.0f, 1.0f, 0.0f);
    glRotatef(bankAngle, 0.0f, 0.0f, 1.0f);

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    GLfloat ambientBoost[] = { 0.5f, 0.5f, 0.5f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientBoost);

    glColor3f(0.2f, 0.5f, 0.2f); // Verde militar
    glPushMatrix();
    glScalef(1.8f, 1.2f, 3.5f);
    glutSolidSphere(2.0f, 12, 8);
    glPopMatrix();

    glColor3f(0.15f, 0.4f, 0.15f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -5.5f);
    glScalef(1.2f, 1.0f, 1.5f);
    glutSolidSphere(1.5f, 10, 6);
    glPopMatrix();

    glColor3f(0.4f, 0.7f, 0.9f);
    glPushMatrix();
    glTranslatef(0.0f, 0.8f, -4.5f);
    glScalef(1.4f, 1.0f, 1.8f);
    glutSolidSphere(1.2f, 10, 6);
    glPopMatrix();

    glColor3f(0.2f, 0.5f, 0.2f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);

    glVertex3f(-0.6f, 0.0f, 5.0f);
    glVertex3f(0.6f, 0.0f, 5.0f);
    glVertex3f(0.4f, 0.0f, 13.0f);
    glVertex3f(-0.4f, 0.0f, 13.0f);
    glEnd();

    glBegin(GL_QUADS);
    glNormal3f(0, 0, 1);
    glVertex3f(-0.6f, -0.8f, 5.0f);
    glVertex3f(0.6f, -0.8f, 5.0f);
    glVertex3f(0.4f, 0.8f, 13.0f);
    glVertex3f(-0.4f, 0.8f, 13.0f);
    glEnd();

    glColor3f(0.15f, 0.4f, 0.15f);
    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-5.0f, 0.0f, 11.0f);
    glVertex3f(5.0f, 0.0f, 11.0f);
    glVertex3f(4.0f, 0.0f, 13.0f);
    glVertex3f(-4.0f, 0.0f, 13.0f);
    glEnd();

    glPushMatrix();
    glTranslatef(0.0f, 2.8f, 0.0f);
    glRotatef(rotorAngle, 0.0f, 1.0f, 0.0f);

    glColor3f(0.1f, 0.1f, 0.1f);

    glPushMatrix();
    glScalef(0.5f, 0.3f, 0.5f);
    glutSolidSphere(1.0f, 8, 6);
    glPopMatrix();

    for (int i = 0; i < 3; i++) {
        glPushMatrix();
        glRotatef(i * 120.0f, 0.0f, 1.0f, 0.0f);
        glColor3f(0.08f, 0.08f, 0.08f);
        glBegin(GL_QUADS);
        glNormal3f(0, 1, 0);
        glVertex3f(-0.5f, 0.0f, 0.5f);
        glVertex3f(0.5f, 0.0f, 0.5f);
        glVertex3f(0.8f, 0.0f, 12.0f);
        glVertex3f(-0.8f, 0.0f, 12.0f);
        glEnd();
        glPopMatrix();
    }
    glPopMatrix();

    glPushMatrix();
    glTranslatef(0.8f, 0.5f, 12.5f);
    glRotatef(tailRotorAngle, 1.0f, 0.0f, 0.0f);

    glColor3f(0.1f, 0.1f, 0.1f);
    glPushMatrix();
    glScalef(0.2f, 0.2f, 0.2f);
    glutSolidSphere(1.0f, 6, 4);
    glPopMatrix();

    for (int i = 0; i < 2; i++) {
        glPushMatrix();
        glRotatef(i * 90.0f, 1.0f, 0.0f, 0.0f);
        glColor3f(0.08f, 0.08f, 0.08f);
        glBegin(GL_QUADS);
        glNormal3f(1, 0, 0);
        glVertex3f(0.1f, -0.3f, 0.0f);
        glVertex3f(0.1f, 0.3f, 0.0f);
        glVertex3f(0.1f, 0.5f, 3.5f);
        glVertex3f(0.1f, -0.5f, 3.5f);
        glEnd();
        glPopMatrix();
    }
    glPopMatrix();

    glColor3f(0.1f, 0.1f, 0.1f);

    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(-3.0f, -2.5f, -4.0f);
    glVertex3f(-2.5f, -2.5f, -4.0f);
    glVertex3f(-2.5f, -2.5f, 5.0f);
    glVertex3f(-3.0f, -2.5f, 5.0f);
    glEnd();

    glBegin(GL_QUADS);
    glNormal3f(0, 1, 0);
    glVertex3f(2.5f, -2.5f, -4.0f);
    glVertex3f(3.0f, -2.5f, -4.0f);
    glVertex3f(3.0f, -2.5f, 5.0f);
    glVertex3f(2.5f, -2.5f, 5.0f);
    glEnd();

    glColor3f(0.15f, 0.15f, 0.15f);
    float suportPozitii[2][2] = { {-2.75f, -3.5f}, {2.75f, -3.5f} };
    float suportZ[2] = { -2.5f, 3.5f };
    for (int s = 0; s < 2; s++) {
        for (int z = 0; z < 2; z++) {
            glBegin(GL_LINES);
            glVertex3f(0.0f, -1.5f, suportZ[z]);
            glVertex3f(suportPozitii[s][0], -2.5f, suportZ[z]);
            glEnd();
        }
    }

    glDisable(GL_LIGHTING);
    float timp = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    if (fmod(timp, 1.0f) < 0.5f) {
        glColor3f(1.0f, 0.0f, 0.0f);
        glPushMatrix();
        glTranslatef(-3.0f, -2.5f, 0.0f);
        glutSolidSphere(0.3f, 6, 6);
        glPopMatrix();
    }

    if (fmod(timp, 1.0f) > 0.5f) {
        glColor3f(0.0f, 1.0f, 0.0f);
        glPushMatrix();
        glTranslatef(3.0f, -2.5f, 0.0f);
        glutSolidSphere(0.3f, 6, 6);
        glPopMatrix();
    }

    if (fmod(timp, 0.5f) < 0.1f) {
        glColor3f(1.0f, 1.0f, 1.0f);
        glPushMatrix();
        glTranslatef(0.0f, 3.5f, 0.0f);
        glutSolidSphere(0.4f, 6, 6);
        glPopMatrix();
    }

    glEnable(GL_LIGHTING);

    GLfloat ambientOrig[] = { 0.15f, 0.15f, 0.2f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientOrig);
    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}

void DrawAirplane(float x, float y, float z, float angle, float bankAngle) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glRotatef(-angle, 0.0f, 1.0f, 0.0f);   
    glRotatef(bankAngle, 0.0f, 0.0f, 1.0f); 
    glRotatef(-8.0f, 1.0f, 0.0f, 0.0f);     

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);


    glColor3f(0.85f, 0.85f, 0.9f); 
    glPushMatrix();
    glScalef(1.0f, 1.0f, 6.0f);    
    glutSolidSphere(2.0f, 12, 8);
    glPopMatrix();

    //Varful 
    glColor3f(0.75f, 0.75f, 0.8f);
    glPushMatrix();
    glTranslatef(0.0f, 0.0f, -8.0f);
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glutSolidCone(1.5f, 4.0f, 10, 4);
    glPopMatrix();

    // Coada verticala
    glColor3f(0.2f, 0.3f, 0.7f); 
    glBegin(GL_TRIANGLES);
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(0.0f, 2.0f, 8.0f);
    glVertex3f(0.0f, 7.0f, 5.0f);
    glVertex3f(0.0f, 2.0f, 4.0f);
    glEnd();

    // Coada orizontala 
    glColor3f(0.85f, 0.85f, 0.9f);
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-5.0f, 1.5f, 8.0f);
    glVertex3f(5.0f, 1.5f, 8.0f);
    glVertex3f(3.0f, 1.5f, 5.0f);
    glVertex3f(-3.0f, 1.5f, 5.0f);
    glEnd();

    glColor3f(0.85f, 0.85f, 0.9f);
    // Aripa stanga
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-18.0f, 0.0f, 2.0f);  
    glVertex3f(-18.0f, 0.0f, 4.0f);
    glVertex3f(-1.5f, 0.0f, 1.0f);   
    glVertex3f(-1.5f, 0.0f, -2.0f);  
    glEnd();

    // Aripa dreapta
    glBegin(GL_QUADS);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(1.5f, 0.0f, -2.0f);
    glVertex3f(1.5f, 0.0f, 1.0f);
    glVertex3f(18.0f, 0.0f, 4.0f);
    glVertex3f(18.0f, 0.0f, 2.0f);
    glEnd();

    glColor3f(0.3f, 0.3f, 0.35f); 
    float engineOffsets[2] = { -8.0f, 8.0f };
    for (int e = 0; e < 2; e++) {
        glPushMatrix();
        glTranslatef(engineOffsets[e], -1.5f, 0.0f);
        glScalef(1.0f, 1.0f, 2.5f);
        glutSolidSphere(1.2f, 8, 6);
        glPopMatrix();
    }

    glDisable(GL_LIGHTING);
    float timp = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;

    if (fmod(timp, 1.0f) < 0.5f) {
        glColor3f(1.0f, 0.0f, 0.0f);
        glPushMatrix();
        glTranslatef(-18.0f, 0.5f, 3.0f);
        glutSolidSphere(0.4f, 6, 6);
        glPopMatrix();
    }

    if (fmod(timp, 1.0f) > 0.5f) {
        glColor3f(0.0f, 1.0f, 0.0f);
        glPushMatrix();
        glTranslatef(18.0f, 0.5f, 3.0f);
        glutSolidSphere(0.4f, 6, 6);
        glPopMatrix();
    }

    if (fmod(timp, 0.5f) < 0.1f) {
        glColor3f(1.0f, 1.0f, 1.0f);
        glPushMatrix();
        glTranslatef(0.0f, -2.5f, 0.0f);
        glutSolidSphere(0.5f, 6, 6);
        glPopMatrix();
    }

    glEnable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glPopMatrix();
}

void DrawCircularCar(float cx, float cz, float circAngleDeg, int colorIdx) {
    glPushMatrix();
    glTranslatef(cx, -9.1f, cz);

    float headingDeg = -circAngleDeg; 
    glRotatef(headingDeg, 0.0f, 1.0f, 0.0f);
    glScalef(1.0f, 0.9f, 1.0f);

    glDisable(GL_TEXTURE_2D);
    glEnable(GL_LIGHTING);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    GLfloat ambientBoost[] = { 0.6f, 0.6f, 0.6f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientBoost);

    float colors[4][3] = {
		{0.8f, 0.1f, 0.1f},   // 0 = Rosu
        {0.9f, 0.6f, 0.0f},   // 1 = Galben
		{0.1f, 0.3f, 0.8f},   // 2 = Albastru
        {0.1f, 0.7f, 0.1f}    // 3 = Verde
    };
    int idx = colorIdx % 4;
    glColor3f(colors[idx][0], colors[idx][1], colors[idx][2]);

    glBegin(GL_QUADS);
    glNormal3f(0, 0, -1);
    // Fata
    glVertex3f(-2.0f, 0.0f, -4.0f); glVertex3f(2.0f, 0.0f, -4.0f);
    glVertex3f(2.0f, 1.5f, -4.0f);  glVertex3f(-2.0f, 1.5f, -4.0f);
    glNormal3f(0, 0, 1);
    // Spate
    glVertex3f(2.0f, 0.0f, 4.0f);  glVertex3f(-2.0f, 0.0f, 4.0f);
    glVertex3f(-2.0f, 1.5f, 4.0f); glVertex3f(2.0f, 1.5f, 4.0f);
    glNormal3f(-1, 0, 0);
    // Stanga
    glVertex3f(-2.0f, 0.0f, 4.0f);  glVertex3f(-2.0f, 0.0f, -4.0f);
    glVertex3f(-2.0f, 1.5f, -4.0f); glVertex3f(-2.0f, 1.5f, 4.0f);
    glNormal3f(1, 0, 0);
    // Dreapta
    glVertex3f(2.0f, 0.0f, -4.0f); glVertex3f(2.0f, 0.0f, 4.0f);
    glVertex3f(2.0f, 1.5f, 4.0f);  glVertex3f(2.0f, 1.5f, -4.0f);
    glNormal3f(0, 1, 0);
    // Sus
    glVertex3f(-2.0f, 1.5f, -4.0f); glVertex3f(2.0f, 1.5f, -4.0f);
    glVertex3f(2.0f, 1.5f, 4.0f);   glVertex3f(-2.0f, 1.5f, 4.0f);
    glEnd();

    // Habitaclu
    glColor3f(0.12f, 0.12f, 0.12f);
    glBegin(GL_QUADS);
    glNormal3f(0, 0.7f, -0.7f);
    glVertex3f(-1.6f, 1.5f, -2.0f); glVertex3f(1.6f, 1.5f, -2.0f);
    glVertex3f(1.4f, 3.0f, -1.5f);  glVertex3f(-1.4f, 3.0f, -1.5f);
    glNormal3f(0, 1, 0);
    glVertex3f(-1.4f, 3.0f, -1.5f); glVertex3f(1.4f, 3.0f, -1.5f);
    glVertex3f(1.4f, 3.0f, 1.5f);   glVertex3f(-1.4f, 3.0f, 1.5f);
    glNormal3f(0, 0.7f, 0.7f);
    glVertex3f(-1.4f, 3.0f, 1.5f);  glVertex3f(1.4f, 3.0f, 1.5f);
    glVertex3f(1.6f, 1.5f, 2.5f);   glVertex3f(-1.6f, 1.5f, 2.5f);
    glNormal3f(-1, 0, 0);
    glVertex3f(-1.6f, 1.5f, 2.5f);  glVertex3f(-1.6f, 1.5f, -2.0f);
    glVertex3f(-1.4f, 3.0f, -1.5f); glVertex3f(-1.4f, 3.0f, 1.5f);
    glNormal3f(1, 0, 0);
    glVertex3f(1.6f, 1.5f, -2.0f);  glVertex3f(1.6f, 1.5f, 2.5f);
    glVertex3f(1.4f, 3.0f, 1.5f);   glVertex3f(1.4f, 3.0f, -1.5f);
    glEnd();

    // Faruri fata
    glDisable(GL_LIGHTING);
    glColor3f(1.0f, 1.0f, 0.7f);
    glBegin(GL_QUADS);
    glVertex3f(-1.8f, 0.5f, -4.01f); glVertex3f(-1.0f, 0.5f, -4.01f);
    glVertex3f(-1.0f, 1.1f, -4.01f); glVertex3f(-1.8f, 1.1f, -4.01f);
    glVertex3f(1.0f, 0.5f, -4.01f); glVertex3f(1.8f, 0.5f, -4.01f);
    glVertex3f(1.8f, 1.1f, -4.01f); glVertex3f(1.0f, 1.1f, -4.01f);
    glEnd();

    // Stopuri spate
    glColor3f(1.0f, 0.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex3f(-1.8f, 0.5f, 4.01f); glVertex3f(-1.0f, 0.5f, 4.01f);
    glVertex3f(-1.0f, 1.1f, 4.01f); glVertex3f(-1.8f, 1.1f, 4.01f);
    glVertex3f(1.0f, 0.5f, 4.01f); glVertex3f(1.8f, 0.5f, 4.01f);
    glVertex3f(1.8f, 1.1f, 4.01f); glVertex3f(1.0f, 1.1f, 4.01f);
    glEnd();
    glEnable(GL_LIGHTING);

    DrawWheel(1.8f, -0.5f, -2.5f);
    DrawWheel(-1.8f, -0.5f, -2.5f);
    DrawWheel(1.8f, -0.5f, 2.5f);
    DrawWheel(-1.8f, -0.5f, 2.5f);

    glEnable(GL_TEXTURE_2D);
    GLfloat ambientOrig[] = { 0.15f, 0.15f, 0.2f, 1.0f };
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambientOrig);

    glDisable(GL_COLOR_MATERIAL);
    glEnable(GL_TEXTURE_2D);
    glPopMatrix();
    glPopMatrix();
}

void DrawCar() {
    glPushMatrix();

    glTranslatef(carX, -8.5f, carZ);
    glRotatef(carAngle, 0.0f, 1.0f, 0.0f);

    glScalef(1.4f, 1.2f, 1.1f);

    GLfloat mat_specular[] = { 1.0f, 1.0f, 1.0f, 1.0f };
    GLfloat mat_shininess[] = { 128.0f };
    glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
    glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texCar);
    glColor3f(0.7f, 0.7f, 0.75f);

    glBegin(GL_QUADS);
    // LATERALA STANGA
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(-2.5f, 0.0f, 5.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(-1.5f, 0.0f, -5.0f);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(-1.5f, 1.8f, -5.0f);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(-2.5f, 2.0f, 5.0f);

    // LATERALA DREAPTA
    glNormal3f(1.0f, 0.0f, 0.0f);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(1.5f, 0.0f, -5.0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(2.5f, 0.0f, 5.0f);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(2.5f, 2.0f, 5.0f);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(1.5f, 1.8f, -5.0f);

    // FATA MASINII
    glNormal3f(0.0f, 0.0f, -1.0f);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(-1.5f, 0.0f, -5.0f);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(1.5f, 0.0f, -5.0f);
    glTexCoord2f(1.0f, 0.6f); glVertex3f(1.5f, 1.8f, -5.0f);
    glTexCoord2f(0.0f, 0.6f); glVertex3f(-1.5f, 1.8f, -5.0f);

    // SPATELE 
    glNormal3f(0.0f, 0.0f, 1.0f);
    glTexCoord2f(0.0f, 0.5f); glVertex3f(2.5f, 0.0f, 5.0f);
    glTexCoord2f(1.0f, 0.5f); glVertex3f(-2.5f, 0.0f, 5.0f);
    glTexCoord2f(1.0f, 0.6f); glVertex3f(-2.5f, 2.0f, 5.0f);
    glTexCoord2f(0.0f, 0.6f); glVertex3f(2.5f, 2.0f, 5.0f);
    glEnd();

    glDisable(GL_TEXTURE_2D);
    glColor3f(0.05f, 0.05f, 0.05f); // Geamuri fumurii

    glBegin(GL_QUADS);
    // PARBRIZ 
    glNormal3f(0.0f, 0.7f, -0.7f);
    glVertex3f(-1.5f, 1.8f, -5.0f);
    glVertex3f(1.5f, 1.8f, -5.0f);
    glVertex3f(1.2f, 4.5f, -0.5f);  // Start Plafon
    glVertex3f(-1.2f, 4.5f, -0.5f);

    // PLAFON 
    glNormal3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-1.2f, 4.5f, -0.5f);
    glVertex3f(1.2f, 4.5f, -0.5f);
    glVertex3f(1.2f, 4.5f, 1.5f);   // Sfarsit Plafon
    glVertex3f(-1.2f, 4.5f, 1.5f);

    // LUNETA 
    glNormal3f(0.0f, 0.7f, 0.7f);
    glVertex3f(-1.2f, 4.5f, 1.5f);
    glVertex3f(1.2f, 4.5f, 1.5f);
    glVertex3f(2.0f, 2.0f, 5.0f);
    glVertex3f(-2.0f, 2.0f, 5.0f);

    // GEAMURI LATERALE
    // Stanga
    glNormal3f(-1.0f, 0.0f, 0.0f);
    glVertex3f(-1.5f, 1.8f, -5.0f);
    glVertex3f(-2.5f, 2.0f, 5.0f);
    glVertex3f(-1.2f, 4.5f, 1.5f);
    glVertex3f(-1.2f, 4.5f, -0.5f);

    // Dreapta
    glNormal3f(1.0f, 0.0f, 0.0f);
    glVertex3f(1.5f, 1.8f, -5.0f);
    glVertex3f(2.5f, 2.0f, 5.0f);
    glVertex3f(1.2f, 4.5f, 1.5f);
    glVertex3f(1.2f, 4.5f, -0.5f);
    glEnd();

    glDisable(GL_LIGHTING);

    glColor3f(1.0f, 1.0f, 0.8f);
    glPushMatrix();
    glTranslatef(0.0f, 1.75f, -5.05f);
    glScalef(3.0f, 0.08f, 0.1f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glColor3f(1.0f, 0.0f, 0.0f);
    glPushMatrix();
    glTranslatef(0.0f, 1.85f, 5.05f);
    glScalef(4.2f, 0.12f, 0.1f);
    glutSolidCube(1.0f);
    glPopMatrix();

    glEnable(GL_LIGHTING);

    DrawWheel(2.2f, -0.7f, -3.0f);  // Fata Dreapta
    DrawWheel(-2.2f, -0.7f, -3.0f); // Fata Stanga
    DrawWheel(2.2f, -0.7f, 3.0f);   // Spate Dreapta
    DrawWheel(-2.2f, -0.7f, 3.0f);  // Spate Stanga

    glDisable(GL_COLOR_MATERIAL);
    glPopMatrix();
}


bool checkCarCollision(float nextX, float nextZ) {
    float carW = 3.5f;
    float carL = 5.5f;

    BoundingBox carBox;
    carBox.minX = nextX - carW;
    carBox.maxX = nextX + carW;
    carBox.minZ = nextZ - carL;
    carBox.maxZ = nextZ + carL;

    // Coliziune cu cladiri
    for (const auto& building : buildingBoxes) {
        if (carBox.minX < building.maxX && carBox.maxX > building.minX &&
            carBox.minZ < building.maxZ && carBox.maxZ > building.minZ) {
            return true;
        }
    }

    // Coliziune cu stalpi
    for (const auto& lamp : lampBoxes) {
        if (carBox.minX < lamp.maxX && carBox.maxX > lamp.minX &&
            carBox.minZ < lamp.maxZ && carBox.maxZ > lamp.minZ) {
            return true;
        }
    }

    // Coliziune cu muntele
    float terrainH = getTerrainHeight(nextX, nextZ);
    if (terrainH > 2.5f) {
        return true;
    }

    return false;
}


void UpdateMovingObjects(float dt) {
    // ==================== MASINI GIRATORIU ====================
    for (int i = 0; i < 3; i++) {
        circCars[i].angle -= circCars[i].speed;
        if (circCars[i].angle < 0.0f)
            circCars[i].angle += 360.0f;
    }

    // ==================== AVION - MASINA DE STARI ====================
    airplane.stateTimer -= dt;

    switch (airplane.state) {

    case AP_PARCAT:

        airplane.speed = 0.0f;
        airplane.y = -9.0f;
        airplane.pitch = 0.0f;
        airplane.bankAngle = 0.0f;
        if (airplane.stateTimer <= 0.0f) {

            airplane.state = AP_DECOLARE;
            airplane.stateTimer = 6.0f; 
            airplane.angle = RUNWAY_ANGLE;
            printf("Avion: DECOLARE!\n");
        }
        break;

    case AP_DECOLARE:
    {
        airplane.speed += 0.06f;
        if (airplane.speed > 4.5f) airplane.speed = 4.5f;

        float rad = airplane.angle * PI / 180.0f;
        airplane.x += sin(rad) * airplane.speed;
        airplane.z -= cos(rad) * airplane.speed;

        if (airplane.speed > 2.5f) {
            airplane.y += airplane.speed * 0.4f;
            airplane.pitch = -18.0f; 
        }
        else {
            airplane.pitch = 0.0f; 
        }

        if (airplane.y >= 150.0f) {
            airplane.y = 150.0f;
            airplane.pitch = 0.0f;
            airplane.state = AP_ZBOR;
            airplane.stateTimer = 20.0f + (rand() % 20);
            airplane.timer = 5.0f;
            printf("Avion: ZBOR!\n");
        }
        break;
    }

    case AP_ZBOR:
        airplane.timer -= dt;
        if (airplane.timer <= 0.0f) {
            float deviere = -60.0f + (rand() % 121);
            airplane.targetAngle = airplane.angle + deviere;
            airplane.timer = 4.0f + (rand() % 40) * 0.1f;
        }

        {
            float diff = airplane.targetAngle - airplane.angle;
            while (diff > 180.0f)  diff -= 360.0f;
            while (diff < -180.0f) diff += 360.0f;

            float turnRate = 15.0f * dt;
            if (fabs(diff) < turnRate) {
                airplane.angle = airplane.targetAngle;
                airplane.bankAngle = 0.0f;
            }
            else {
                airplane.angle += (diff > 0 ? turnRate : -turnRate);
                airplane.bankAngle = (diff > 0 ? -20.0f : 20.0f);
            }
        }

        if (airplane.angle > 360.0f) airplane.angle -= 360.0f;
        if (airplane.angle < 0.0f)   airplane.angle += 360.0f;

        {
            float rad = airplane.angle * PI / 180.0f;
            airplane.x += sin(rad) * airplane.speed;
            airplane.z -= cos(rad) * airplane.speed;
        }

        // Oscilatie altitudine
        {
            float timp = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
            airplane.y = 150.0f + sin(timp * 0.3f) * 8.0f;
        }

        // Mentine in zona hartii
        {
            float distFata = sqrt(airplane.x * airplane.x + airplane.z * airplane.z);

            if (distFata > 600.0f) {
                float urgenta = (distFata - 600.0f) / 200.0f;
                if (urgenta > 1.0f) urgenta = 1.0f;

                float angleSpre = atan2f(-airplane.x, airplane.z) * 180.0f / PI;
                airplane.targetAngle = angleSpre;
                airplane.timer = 2.0f;

                if (distFata > 750.0f) {
                    float diff = angleSpre - airplane.angle;
                    while (diff > 180.0f)  diff -= 360.0f;
                    while (diff < -180.0f) diff += 360.0f;
                    airplane.angle += diff * urgenta * 0.1f;
                }
            }

            if (distFata > 850.0f) {
                airplane.x *= 0.95f;
                airplane.z *= 0.95f;
            }
        }

        if (airplane.stateTimer <= 0.0f) {
            airplane.state = AP_ATERIZARE;

            airplane.targetAngle = atan2f(
                RUNWAY_X - airplane.x,
                -(RUNWAY_Z - airplane.z)
            ) * 180.0f / PI;
            airplane.stateTimer = 30.0f; 
            printf("Avion: ATERIZARE!\n");
        }
        break;

    case AP_ATERIZARE:
    {
        float dx = RUNWAY_X - airplane.x;
        float dz = RUNWAY_Z - airplane.z;
        float dist = sqrt(dx * dx + dz * dz);

        float angleSpre = atan2f(dx, -dz) * 180.0f / PI;

        float diff = angleSpre - airplane.angle;
        while (diff > 180.0f)  diff -= 360.0f;
        while (diff < -180.0f) diff += 360.0f;

        float turnRate = 60.0f * dt; 
        if (fabs(diff) < turnRate) {
            airplane.angle = angleSpre;
            airplane.bankAngle = 0.0f;
        }
        else {
            airplane.angle += (diff > 0 ? turnRate : -turnRate);
            airplane.bankAngle = (diff > 0 ? -25.0f : 25.0f);
        }

        // Viteza constanta
        airplane.speed = 3.0f;

        // Miscare
        float rad = airplane.angle * PI / 180.0f;
        airplane.x += sin(rad) * airplane.speed;
        airplane.z -= cos(rad) * airplane.speed;

        float targetY = -9.0f + (dist / 500.0f) * 159.0f;
        if (targetY < -9.0f) targetY = -9.0f;
        if (targetY > airplane.y) targetY = airplane.y; 

        float lerpSpeed = (dist < 100.0f) ? 0.08f : 0.04f;
        airplane.y -= 0.3f; 
        if (airplane.y < targetY) airplane.y = targetY;

        airplane.pitch = 5.0f;

        // Aterizat
        if (airplane.y <= -8.5f || dist < 20.0f) {
            airplane.x = RUNWAY_X;
            airplane.z = RUNWAY_Z;
            airplane.y = -9.0f;
            airplane.speed = 0.0f;
            airplane.pitch = 0.0f;
            airplane.bankAngle = 0.0f;
            airplane.angle = RUNWAY_ANGLE;
            airplane.state = AP_PARCAT;
            airplane.stateTimer = 6.0f;
            printf("Avion: PARCAT!\n");
        }

        // Timeout - daca dureaza prea mult, forteaza aterizarea
        if (airplane.stateTimer <= 0.0f) {
            airplane.y -= 1.0f; 
            if (airplane.y <= -9.0f) {
                airplane.x = RUNWAY_X;
                airplane.z = RUNWAY_Z;
                airplane.y = -9.0f;
                airplane.speed = 0.0f;
                airplane.pitch = 0.0f;
                airplane.bankAngle = 0.0f;
                airplane.angle = RUNWAY_ANGLE;
                airplane.state = AP_PARCAT;
                airplane.stateTimer = 6.0f;
                printf("Avion: PARCAT fortat!\n");
            }
            airplane.stateTimer = 0.1f; 
        }
        break;
    }

    // Timeout - daca nu gaseste pista
    if (airplane.stateTimer <= 0.0f) {
        airplane.targetAngle = atan2f(
            RUNWAY_X - airplane.x,
            -(RUNWAY_Z - airplane.z)
        ) * 180.0f / PI;
        airplane.stateTimer = 20.0f;
    }
    break;
    }

    // ==================== ELICOPTER ====================
    helicopter.rotorAngle += 15.0f;
    if (helicopter.rotorAngle >= 360.0f) helicopter.rotorAngle -= 360.0f;

    helicopter.tailRotorAngle += 25.0f;
    if (helicopter.tailRotorAngle >= 360.0f) helicopter.tailRotorAngle -= 360.0f;

    float timpH = glutGet(GLUT_ELAPSED_TIME) / 1000.0f;
    helicopter.y = 60.0f + sin(timpH * 0.8f) * 5.0f;

    helicopter.timer -= dt;
    if (helicopter.timer <= 0.0f) {
        float deviere = -90.0f + (rand() % 181);
        helicopter.targetAngle = helicopter.angle + deviere;
        helicopter.timer = 3.0f + (rand() % 50) * 0.1f;
    }

    {
        float diff = helicopter.targetAngle - helicopter.angle;
        while (diff > 180.0f)  diff -= 360.0f;
        while (diff < -180.0f) diff += 360.0f;

        float turnRate = 20.0f * dt;
        if (fabs(diff) < turnRate) {
            helicopter.angle = helicopter.targetAngle;
            helicopter.bankAngle = 0.0f;
        }
        else {
            helicopter.angle += (diff > 0 ? turnRate : -turnRate);
            helicopter.bankAngle = (diff > 0 ? -15.0f : 15.0f);
        }
    }

    helicopter.angle = fmod(helicopter.angle + 360.0f, 360.0f);

    // Miscare
    float hRad = helicopter.angle * PI / 180.0f;
    helicopter.x += sin(hRad) * helicopter.speed;
    helicopter.z -= cos(hRad) * helicopter.speed;

    // Mentine in zona hartii
    {
        float distH = sqrt(helicopter.x * helicopter.x + helicopter.z * helicopter.z);
        if (distH > 600.0f) {
            float angleSpre = atan2f(-helicopter.x, helicopter.z) * 180.0f / PI;
            helicopter.targetAngle = angleSpre;
            helicopter.timer = 2.0f;
        }
        if (distH > 800.0f) {
            helicopter.x *= 0.97f;
            helicopter.z *= 0.97f;
        }
    }
}

void update(int value) {

    if (!objectsInitialized) {
        InitMovingObjects();
        objectsInitialized = true;
    }

    float dt = 0.016f;

    if (isInCar) {

        if (keys['w'] || keys['W']) {
            carSpeed += 0.05f;
            if (carSpeed > 5.0f) carSpeed = 5.0f;
        }
        else if (keys['s'] || keys['S']) {
            carSpeed -= 0.05f;
            if (carSpeed < -1.0f) carSpeed = -1.0f;
        }
        else {
            carSpeed *= 0.95f;
        }

        if (fabs(carSpeed) > 0.1f) {
            if (keys['a'] || keys['A']) carAngle += 2.0f;
            if (keys['d'] || keys['D']) carAngle -= 2.0f;
        }

        float rad = carAngle * PI / 180.0f;

        float nextX = carX - sin(rad) * carSpeed;
        float nextZ = carZ - cos(rad) * carSpeed;

        if (!checkCarCollision(nextX, nextZ)) {

            carX = nextX;
            carZ = nextZ;
        }
        else {
            carSpeed = -carSpeed * 0.6f;

            carX += sin(rad) * 1.5f;
            carZ += cos(rad) * 1.5f;
        }

        float camDist = 55.0f;
        camX = carX + sin(rad) * camDist;
        camZ = carZ + cos(rad) * camDist;
        camY = -10.0f + 15.0f;

        camAngleY = -carAngle;
        camAngleX = 15.0f;
    }

    UpdateMovingObjects(dt);

    glutPostRedisplay();
    glutTimerFunc(16, update, 0);
}

/* ==================== DISPLAY & LOGIC ==================== */

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glRotatef(camAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(camAngleY, 0.0f, 1.0f, 0.0f);

    DrawSkybox();

    glTranslatef(-camX, -camY, -camZ);

    SetupLighting();

    DrawRelief();
    DrawCircuit();
    DrawRunway();      
    DrawRoadMarkings();
    DrawLake(300.0f, 300.0f, 80.0f);
    DrawLampPost(150.0f, 150.0f, 100.0f, false);
    DrawEnvironment();
    DrawCar();
    // Avion 
    DrawAirplane(airplane.x, airplane.y, airplane.z, airplane.angle, airplane.bankAngle);
    //Elicopter
    DrawHelicopter(helicopter.x, helicopter.y, helicopter.z,
        helicopter.angle, helicopter.bankAngle,
        helicopter.rotorAngle, helicopter.tailRotorAngle);

    // Masini in sensul giratoriu 
    for (int i = 0; i < 3; i++) {
        float rad = circCars[i].angle * PI / 180.0f;
        float cx = cos(rad) * circCars[i].radius;
        float cz = sin(rad) * circCars[i].radius;
        DrawCircularCar(cx, cz, circCars[i].angle, circCars[i].colorIdx);
    }
    glutSwapBuffers();
}

/* ==================== INPUT HANDLERS ==================== */

void keyboard(unsigned char key, int x, int y) {
    keys[key] = true;

    switch (key) {
    case 'e': case 'E':
        isInCar = !isInCar;
        if (!isInCar) camY = 3.0f;
        break;
    case 27: // ESC
        if (isInCar) isInCar = false;
        else exit(0);
        break;
    }

    if (!isInCar) {
        float speed = 3.0f;
        float rad = camAngleY * PI / 180.0f;
        switch (key) {
        case 'w': case 'W': camX += sin(rad) * speed; camZ -= cos(rad) * speed; break;
        case 's': case 'S': camX -= sin(rad) * speed; camZ += cos(rad) * speed; break;
        case 'a': case 'A': camX -= cos(rad) * speed; camZ -= sin(rad) * speed; break;
        case 'd': case 'D': camX += cos(rad) * speed; camZ += sin(rad) * speed; break;
        case 'r': case 'R': camY += speed; break;
        case 'f': case 'F': camY -= speed; break;
        }
    }

    glutPostRedisplay();
}

void keyboardUp(unsigned char key, int x, int y) {
    keys[key] = false;
}


void mouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) { mouseDown = true; lastX = x; lastY = y; }
        else mouseDown = false;
    }
}

void motion(int x, int y) {
    if (mouseDown) {
        camAngleY += (x - lastX) * 0.2f;
        camAngleX += (y - lastY) * 0.2f;
        if (camAngleX > 85.0f) camAngleX = 85.0f;
        if (camAngleX < -85.0f) camAngleX = -85.0f;
        lastX = x; lastY = y;
        glutPostRedisplay();
    }
}

void init() {
    glEnable(GL_DEPTH_TEST);

    texFront = LoadTexture("front1.png", false);
    texBack = LoadTexture("back1.png", false);
    texLeft = LoadTexture("left1.png", false);
    texRight = LoadTexture("right1.png", false);
    texTop = LoadTexture("top1.png", false);
    texBottom = LoadTexture("bottom1.png", false);
    texGrass = LoadTexture("grass005.jpg", true);
    texStone = LoadTexture("rock_texture1.png", true);
    texRoad = LoadTexture("road_texture1.png", true);
    texBuilding = LoadTexture("building.png", true);
    texTree = LoadTexture("tree.png", true);
    texLake = LoadTexture("lake.png", true);
    texShadow = LoadTexture("shadow1.png", false);
    texLamp = LoadTexture("road_texture.png", true);
    //texCar = LoadTexture("car_tex.png", true);

    /* glEnable(GL_FOG);
     GLfloat fogColor[] = { 0.7f, 0.8f, 0.9f, 1.0f };
     glFogfv(GL_FOG_COLOR, fogColor);
     glFogf(GL_FOG_DENSITY, 0.0015f);
     glFogf(GL_FOG_MODE, GL_EXP); */

}

void reshape(int w, int h) {
    if (h == 0) h = 1;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(45.0f, (float)w / h, 1.0f, 3000.0f);
    glMatrixMode(GL_MODELVIEW);
}

int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
    glutInitWindowSize(1280, 720);
    glutCreateWindow("Skybox Imersiv - 6 Texturi");
    init();
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutKeyboardUpFunc(keyboardUp);
    glutTimerFunc(0, update, 0);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMainLoop();
    return 0;
}
#include <freeglut.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define SKYBOX_SIZE 1000.0f
#define PI 3.14159265f

// Definirea constantei pentru compatibilitate cu versiuni vechi de OpenGL
#ifndef GL_CLAMP_TO_EDGE
#define GL_CLAMP_TO_EDGE 0x812F
#endif

// Variabile pentru texturi
GLuint texFront, texBack, texLeft, texRight, texTop, texBottom, texGrass, texStone, texRoad, texBuilding, texTree, texLake;

// Variabile pentru camera (Pozitie si Rotatie)
float camX = 0.0f, camY = 3.0f, camZ = 50.0f;
float camAngleX = 0.0f;
float camAngleY = 0.0f;

// Variabile pentru mouse
int lastX, lastY;
bool mouseDown = false;

/* ==================== ÎNC?RCARE TEXTURI ==================== */
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

/* ==================== DESENARE SCENA ==================== */

void DrawSkybox() {
    float s = SKYBOX_SIZE / 2.0f;
    glDisable(GL_LIGHTING);
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);

    // FA??
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

void DrawRelief() {
    glEnable(GL_TEXTURE_2D);
    float d_densitate = 15.0f;

    auto getH = [](float x, float z) {
        float dist = sqrt(x * x + z * z);

        float h = (float)(cos(dist * 0.02f) * exp(-dist * 0.015f) * 30.0f);

        return (dist > 180.0f) ? 0.0f : h;
        };

    glBindTexture(GL_TEXTURE_2D, texGrass);
    glBegin(GL_QUADS);
    for (float x = -1000.0f; x < 1000.0f; x += 10.0f) {
        for (float z = -1000.0f; z < 1000.0f; z += 10.0f) {
            float h = getH(x, z);
            float dist = sqrt(x * x + z * z);

            if (h <= 2.5f || dist > 110.0f) {
                glTexCoord2f(x / d_densitate, z / d_densitate); glVertex3f(x, h - 10.0f, z);
                glTexCoord2f((x + 10) / d_densitate, z / d_densitate); glVertex3f(x + 10, getH(x + 10, z) - 10.0f, z);
                glTexCoord2f((x + 10) / d_densitate, (z + 10) / d_densitate); glVertex3f(x + 10, getH(x + 10, z + 10) - 10.0f, z + 10);
                glTexCoord2f(x / d_densitate, (z + 10) / d_densitate); glVertex3f(x, getH(x, z + 10) - 10.0f, z + 10);
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
            float h = getH(x, z);
            float dist = sqrt(x * x + z * z);

            if (h > 2.5f && dist <= 110.0f) {
                glTexCoord2f(x / d_densitate, z / d_densitate); glVertex3f(x, h - 10.0f, z);
                glTexCoord2f((x + 10) / d_densitate, z / d_densitate); glVertex3f(x + 10, getH(x + 10, z) - 10.0f, z);
                glTexCoord2f((x + 10) / d_densitate, (z + 10) / d_densitate); glVertex3f(x + 10, getH(x + 10, z + 10) - 10.0f, z + 10);
                glTexCoord2f(x / d_densitate, (z + 10) / d_densitate); glVertex3f(x, getH(x, z + 10) - 10.0f, z + 10);
            }
        }
    }
    glEnd();
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

void DrawBuilding(float x, float z, float w, float h, float d) {
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
    glColor4f(0.0f, 0.0f, 0.0f, 0.3f); // Negru transparent (umbra)
    glBegin(GL_QUADS);
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

void DrawLampPost(float x, float z) {
    glBindTexture(GL_TEXTURE_2D, texStone);
    glBegin(GL_QUADS);
    // Fata
    glTexCoord2f(0, 0); glVertex3f(x - 0.5f, -10.0f, z + 0.5f);
    glTexCoord2f(1, 0); glVertex3f(x + 0.5f, -10.0f, z + 0.5f);
    glTexCoord2f(1, 1); glVertex3f(x + 0.5f, 20.0f, z + 0.5f);
    glTexCoord2f(0, 1); glVertex3f(x - 0.5f, 20.0f, z + 0.5f);
    // Spate
    glTexCoord2f(0, 0); glVertex3f(x - 0.5f, -10.0f, z - 0.5f);
    glTexCoord2f(1, 0); glVertex3f(x + 0.5f, -10.0f, z - 0.5f);
    glTexCoord2f(1, 1); glVertex3f(x + 0.5f, 20.0f, z - 0.5f);
    glTexCoord2f(0, 1); glVertex3f(x - 0.5f, 20.0f, z - 0.5f);
    // Stanga
    glTexCoord2f(0, 0); glVertex3f(x - 0.5f, -10.0f, z - 0.5f);
    glTexCoord2f(1, 0); glVertex3f(x - 0.5f, -10.0f, z + 0.5f);
    glTexCoord2f(1, 1); glVertex3f(x - 0.5f, 20.0f, z + 0.5f);
    glTexCoord2f(0, 1); glVertex3f(x - 0.5f, 20.0f, z - 0.5f);
    // Dreapta
    glTexCoord2f(0, 0); glVertex3f(x + 0.5f, -10.0f, z + 0.5f);
    glTexCoord2f(1, 0); glVertex3f(x + 0.5f, -10.0f, z - 0.5f);
    glTexCoord2f(1, 1); glVertex3f(x + 0.5f, 20.0f, z - 0.5f);
    glTexCoord2f(0, 1); glVertex3f(x + 0.5f, 20.0f, z + 0.5f);
    glEnd();

    // --- BECUL
    glDisable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 0.5f);
    glBegin(GL_QUADS);
    // Fata
    glVertex3f(x - 1.5f, 18.0f, z + 1.5f); glVertex3f(x + 1.5f, 18.0f, z + 1.5f);
    glVertex3f(x + 1.5f, 21.0f, z + 1.5f); glVertex3f(x - 1.5f, 21.0f, z + 1.5f);
    // Spate
    glVertex3f(x - 1.5f, 18.0f, z - 1.5f); glVertex3f(x + 1.5f, 18.0f, z - 1.5f);
    glVertex3f(x + 1.5f, 21.0f, z - 1.5f); glVertex3f(x - 1.5f, 21.0f, z - 1.5f);
    // Tavan bec
    glVertex3f(x - 1.5f, 21.0f, z + 1.5f); glVertex3f(x + 1.5f, 21.0f, z + 1.5f);
    glVertex3f(x + 1.5f, 21.0f, z - 1.5f); glVertex3f(x - 1.5f, 21.0f, z - 1.5f);
    glEnd();
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);
}

void DrawEnvironment() {
    srand(42);

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
        }
        if (pozitieValida) DrawBuilding(x, z, 40.0f + (rand() % 20), 80.0f + (rand() % 120), 40.0f + (rand() % 20));
    }

    for (int i = 0; i < 150; i++) {
        float tx = (float)(rand() % 1800 - 900);
        float tz = (float)(rand() % 1800 - 900);
        float distC = sqrt(tx * tx + tz * tz);

        if (distC > 160.0f && !(fabsf(tx) < 35.0f) && !(fabsf(tz) < 35.0f)) {
            DrawTree(tx, tz, 15.0f, 20.0f + (rand() % 10));
        }
    }

    for (float d = 160.0f; d < 800.0f; d += 100.0f) {
        DrawLampPost(35.0f, d); 
        DrawLampPost(-35.0f, -d); 
        DrawLampPost(d, 35.0f);  
        DrawLampPost(-d, -35.0f); 
    }
}



/* ==================== DISPLAY & LOGIC ==================== */

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    glRotatef(camAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(camAngleY, 0.0f, 1.0f, 0.0f);

    DrawSkybox();

    glTranslatef(-camX, -camY, -camZ);
    DrawRelief();
    DrawCircuit();
	DrawRoadMarkings();
    DrawLake(300.0f, 300.0f, 80.0f); 
	DrawLampPost(150.0f, 150.0f); 
	DrawEnvironment();

    glutSwapBuffers();
}

/* ==================== INPUT HANDLERS ==================== */

void keyboard(unsigned char key, int x, int y) {
    float speed = 3.0f;
    float rad = camAngleY * PI / 180.0f;
    switch (key) {
    case 'w': case 'W': camX += sin(rad) * speed; camZ -= cos(rad) * speed; break;
    case 's': case 'S': camX -= sin(rad) * speed; camZ += cos(rad) * speed; break;
    case 'a': case 'A': camX -= cos(rad) * speed; camZ -= sin(rad) * speed; break;
    case 'd': case 'D': camX += cos(rad) * speed; camZ += sin(rad) * speed; break;


    case 'r': case 'R': camY += speed; break; // Up
    case 'f': case 'F': camY -= speed; break; // Down
    case 27: exit(0); break;
    }
    glutPostRedisplay();
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
    // Adaugam ceata fina pentru a topi terenul în orizont
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
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutMainLoop();
    return 0;
}
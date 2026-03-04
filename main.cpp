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
GLuint texFront, texBack, texLeft, texRight, texTop, texBottom, texGrass, texStone;

// Variabile pentru camera (Pozitie si Rotatie)
float camX = 0.0f, camY = 3.0f, camZ = 50.0f;
float camAngleX = 0.0f;
float camAngleY = 0.0f;

// Variabile pentru mouse
int lastX, lastY;
bool mouseDown = false;

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

void DrawRelief() {
    glEnable(GL_TEXTURE_2D);
    float d_densitate = 15.0f;

    // Definirea functiei de inaltime
    auto getH = [](float x, float z) {
        float dist = sqrt(x * x + z * z);

        // Crestem multiplicatorul la 30.0f pentru un deal mult mai înalt
        // Marim factorul de amortizare (0.015f) pentru ca marginile sa devina plate rapid
        float h = (float)(cos(dist * 0.02f) * exp(-dist * 0.015f) * 30.0f);

        // Fortam marginile sa fie perfect plate la distanta mare
        return (dist > 180.0f) ? 0.0f : h;
        };

    // Desenam iarba (Campia de la baza)
    glBindTexture(GL_TEXTURE_2D, texGrass);
    glBegin(GL_QUADS);
    for (float x = -400.0f; x < 400.0f; x += 10.0f) {
        for (float z = -400.0f; z < 400.0f; z += 10.0f) {
            float h = getH(x, z);
            float dist = sqrt(x * x + z * z);

            // Desenam cu iarba daca suntem la baza dealului sau departe de centru
            if (h <= 2.5f || dist > 110.0f) {
                glTexCoord2f(x / d_densitate, z / d_densitate); glVertex3f(x, h - 10.0f, z);
                glTexCoord2f((x + 10) / d_densitate, z / d_densitate); glVertex3f(x + 10, getH(x + 10, z) - 10.0f, z);
                glTexCoord2f((x + 10) / d_densitate, (z + 10) / d_densitate); glVertex3f(x + 10, getH(x + 10, z + 10) - 10.0f, z + 10);
                glTexCoord2f(x / d_densitate, (z + 10) / d_densitate); glVertex3f(x, getH(x, z + 10) - 10.0f, z + 10);
            }
        }
    }
    glEnd();

    // Desenam stanca
    glBindTexture(GL_TEXTURE_2D, texStone);
    glBegin(GL_QUADS);
    for (float x = -400.0f; x < 400.0f; x += 10.0f) {
        for (float z = -400.0f; z < 400.0f; z += 10.0f) {
            float h = getH(x, z);
            float dist = sqrt(x * x + z * z);

            // Stanca apare doar in centrul muntelui si doar la inaltime
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


/* ==================== DISPLAY & LOGIC ==================== */

void display() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();

    // Rotatie (Mouse)
    glRotatef(camAngleX, 1.0f, 0.0f, 0.0f);
    glRotatef(camAngleY, 0.0f, 1.0f, 0.0f);

    // Skybox fix (nu ne putem apropia de peretii cubului) 
    DrawSkybox();

    // Translatie (WASD) doar pentru teren
    glTranslatef(-camX, -camY, -camZ);
    DrawRelief();

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
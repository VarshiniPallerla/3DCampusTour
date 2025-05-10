#include <Windows.h>
#include<stdio.h>
#include<math.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include<errno.h>
#include<GL\GL.h>
#include<GL\GLU.h>
#include<GL\glut.h>
#include <initializer_list>
// Road 1: Vertical road (left side)
#define ROAD1_X_MIN -40.0f
#define ROAD1_X_MAX -20.0f
#define ROAD1_Z_START 90.0f
#define ROAD1_Z_END -80.0f

// Road 2: Horizontal road (middle)
#define ROAD2_Z_MIN 70.0f
#define ROAD2_Z_MAX 80.0f
#define ROAD2_X_START -20.0f
#define ROAD2_X_END 40.0f

// Road 3: Vertical road (right side)
#define ROAD3_X_MIN 35.0f
#define ROAD3_X_MAX 45.0f
#define ROAD3_Z_START -90.0f
#define ROAD3_Z_END 80.0f

// Road 4: Horizontal road (bottom)
#define ROAD4_Z_MIN -95.0f
#define ROAD4_Z_MAX -80.0f
#define ROAD4_X_START 120.0f
#define ROAD4_X_END -95.0f

// Road 5: Vertical road (left edge)
#define ROAD5_X_MIN -95.0f
#define ROAD5_X_MAX -80.0f
#define ROAD5_Z_START 80.0f
#define ROAD5_Z_END -80.0f

float y = 5, x = -30, z = 109; // initially 5 units south of origin
float deltaMove = 0.0; // initially camera doesn't move

// Camera direction
float lx = 0.0, lz = -1, ly = 0.0; // camera points initially along y-axis
float angle = 0.0; // angle of rotation for the camera direction
float deltaAngle = 0.0; // additional angle change when dragging

// Mouse drag control
int isDragging = 0; // true when dragging
int xDragStart = 0; // records the x-coordinate when dragging starts
int move = 0;
int vertMove = 0;

unsigned char header[54];        // Each BMP file begins by a 54-bytes header
unsigned int dataPos;            // Position in the file where the actual data begins
unsigned int width, height;
unsigned int imageSize;            // = width*height*3
unsigned char* data = NULL;        // Actual RGB data

// ===== New Moving Vehicles and People Setup =====
struct CarColor {
    float r, g, b;
};

CarColor carColors[] = {
    {1.0f, 0.0f, 0.0f},    // Red
    {0.0f, 0.0f, 1.0f},    // Blue
    {0.5f, 0.5f, 0.5f},    // Gray
    {1.0f, 1.0f, 0.0f},    // Yellow
    {0.0f, 1.0f, 0.0f},    // Green
    {1.0f, 0.5f, 0.0f},    // Orange
    {0.7f, 0.7f, 0.7f},    // Silver
    {0.9f, 0.1f, 0.9f},    // Purple
    {0.0f, 0.0f, 0.0f},    // Black
    {1.0f, 1.0f, 1.0f}     // White
};

const int NUM_COLORS = sizeof(carColors) / sizeof(carColors[0]);

// For moving people
struct Person {
    float x, y, z;     // Position
    float speed;       // Walking speed
};

Person people[] = {
    // Existing people
    {-40, 0.2, 40, 0.2}, {20, 0.2, 35, 0.15}, {-60, 0.2, -20, 0.18},
    {-100, 0.2, 0, 0.1}, {60, 0.2, -10, 0.12}, {0, 0.2, -80, 0.1},
    {30, 0.2, 10, 0.13}, {-45, 0.2, 15, 0.15}, {75, 0.2, -40, 0.11}, {-90, 0.2, 25, 0.14},

    // New people on the roads
    // Road 1: Vertical road (x=-40 to -20, z=90 to -80)
    {-40, 0.2, 90, 0.2},/*, {-25, 0.2, 90, 0.18},*/ /*{-30, 0.2, 85, 0.17},*/

    // Road 2: Horizontal road (x=-20 to 40, z=70 to 80)
    {-20, 0.2, 75, 0.15}, {-20, 0.2, 72, 0.14}, {0, 0.2, 78, 0.16},

    // Road 3: Vertical road (x=35 to 45, z=-90 to 80)
    {40, 0.2, -90, 0.19}, {45, 0.2, -90, 0.17}, {35, 0.2, -90, 0.16},

    // Road 4: Horizontal road (x=120 to -95, z=-95 to -80)
    {120, 0.2, -90, 0.13}, {100, 0.2, -90, 0.12}, {50, 0.2, -90, 0.14},

    // Road 5: Vertical road (x=-95 to -80, z=80 to -80)
    {-95, 0.2, 0, 0.14}, {-95, 0.2, -40, 0.15}, {-95, 0.2, 60, 0.16}
};

int numPeople = sizeof(people) / sizeof(Person);
struct Car {
    float x, z;         // Position
    float speed;        // Movement speed
    float r, g, b;      // Color
    int pathIndex;      // Current segment in the path
};

// Define path segments (each segment = road + direction)
enum PathSegment {
    ROAD1_DOWN,   // Road 1: top to bottom
    ROAD2_RIGHT,  // Road 2: left to right
    ROAD3_UP,     // Road 3: bottom to top
    ROAD4_LEFT,   // Road 4: right to left
    ROAD5_DOWN    // Road 5: top to bottom
};

// Initialize cars with paths
Car cars[] = {
    // Road 1: Vertical (x=-40 to -20)
    {-30, 90, 0.2f, 1.0f, 0.0f, 0.0f, ROAD1_DOWN},   // Red car
    {-25, 90, 0.18f, 0.0f, 0.0f, 1.0f, ROAD1_DOWN},  // Blue car

    // Road 2: Horizontal (z=70 to 80)
    {-20, 75, 0.15f, 0.5f, 0.5f, 0.5f, ROAD2_RIGHT}, // Gray car
    {0, 78, 0.16f, 1.0f, 1.0f, 0.0f, ROAD2_RIGHT},   // Yellow car

    // Road 3: Vertical (x=35 to 45)
    {40, -90, 0.19f, 0.0f, 1.0f, 0.0f, ROAD3_UP},    // Green car
    {45, -90, 0.17f, 1.0f, 0.5f, 0.0f, ROAD3_UP},    // Orange car

    // Road 4: Horizontal (z=-95 to -80)
    {120, -90, 0.13f, 0.7f, 0.7f, 0.7f, ROAD4_LEFT}, // Silver car
    {100, -90, 0.12f, 0.9f, 0.1f, 0.9f, ROAD4_LEFT}, // Purple car
};
int numCars = sizeof(cars) / sizeof(Car);

//draw name board
void draw_board()
{
    glColor3f(0.177, 0.564, 1);
    glBegin(GL_QUADS);
    glVertex3f(0, 0, 0);
    glVertex3f(1, 0, 0);
    glVertex3f(1, 2, 0);
    glVertex3f(0, 2, 0);
    glVertex3f(9, 0, 0);
    glVertex3f(10, 0, 0);
    glVertex3f(10, 2, 0);
    glVertex3f(9, 2, 0);
    glEnd();
    glColor3f(0.690, 0.878, 0.901);
    glBegin(GL_QUADS);
    glVertex3f(0, 2, 0);
    glVertex3f(10, 2, 0);
    glVertex3f(10, 4, 0);
    glVertex3f(0, 4, 0);
    glEnd();
}

GLuint loadBMP_custom(const char* imagepath)
{
    // Open the file
    FILE* file = nullptr;
    fopen_s(&file, imagepath, "rb");
    if (!file)
    {
        printf("Image could not be opened\n");
        printf("Error %d \n", errno);
        return 0;
    }
    if (fread(header, 1, 54, file) != 54)
    { // If not 54 bytes read : problem
        printf("Not a correct BMP file\n");
        return false;
    }
    if (header[0] != 'B' || header[1] != 'M')
    {
        printf("Not a correct BMP file\n");
        return 0;
    }
    dataPos = (int)&(header[0x0A]);
    imageSize = (int)&(header[0x22]);
    width = (int)&(header[0x12]);
    height = (int)&(header[0x16]);
    // Some BMP files are misformatted, guess missing information
    if (imageSize == 0)
        imageSize = width * height * 3; // 3 : one byte for each Red, Green and Blue component
    if (dataPos == 0)
        dataPos = 54; // The BMP header is done that way
    // Create a buffer
    data = new unsigned char[imageSize];
    // Read the actual data from the file into the buffer
    fread(data, 1, imageSize, file);
    //Everything is in memory now, the file can be closed
    fclose(file);
}

void draw_map()
{
    GLuint Texture = loadBMP_custom("field.bmp");
    glEnable(GL_TEXTURE_2D);
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Load image, create texture and generate mipmaps
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    glColor3f(0.0f, 1.0f, 0.0f);

    // Draw the field covering the full ground
    glBegin(GL_QUADS);

    // Coordinates for the field (a large plane from -300 to 300 on the X and Z axis)
    glTexCoord2f(-1.0f, -1.0f);   // lower-left corner of the texture
    glVertex3f(-300.0f, -3.0f, 300.0f); // position in world space

    glTexCoord2f(1.0f, -1.0f);    // lower-right corner of the texture
    glVertex3f(300.0f, -3.0f, 300.0f);  // position in world space

    glTexCoord2f(1.0f, 1.0f);     // upper-right corner of the texture
    glVertex3f(300.0f, -3.0f, -300.0f); // position in world space

    glTexCoord2f(-1.0f, 1.0f);    // upper-left corner of the texture
    glVertex3f(-300.0f, -3.0f, -300.0f); // position in world space

    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done

    delete[] data;
    glDeleteTextures(1, &textureID);
    glDisable(GL_TEXTURE_2D);
}

void draw_idol()
{
    GLuint Texture = loadBMP_custom("Vishnu.bmp");
    glEnable(GL_TEXTURE_2D);
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    // Set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // Load image, create texture and generate mipmaps
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f);    glVertex3f(0.0f, .0f, 0.0f);
    glTexCoord2f(1.0f, 0.0f);    glVertex3f(4.0f, 0.0f, 0.0f);
    glTexCoord2f(1.0f, 1.0f);    glVertex3f(4.0f, 5.0f, 0.0f);
    glTexCoord2f(0.0f, 1.0f);    glVertex3f(0.0f, 5.0f, 0.0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0); // Unbind texture when done, so we won't accidentily mess up our texture.
    delete[]data;
    glDeleteTextures(1, &textureID);
    glDisable(GL_TEXTURE_2D);
}

void circle(GLfloat rx, GLfloat ry, GLfloat cx, GLfloat cy) {
    glBegin(GL_POLYGON);
    for (int i = 0; i <= 360; i++) {
        float angle = i * 3.1416 / 180;
        float x = rx * cos(angle);
        float y = ry * sin(angle);
        glVertex2f(x + cx, y + cy);
    }
    glEnd();
}

void draw_vehicle(float x, float y, float z, float scale, float r, float g, float b) {
    glPushMatrix();
    glTranslatef(x, y, z);
    glScalef(scale, scale, scale);

    // Car body
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    // Bottom
    glVertex3f(-1.0f, 0.0f, -1.5f);
    glVertex3f(1.0f, 0.0f, -1.5f);
    glVertex3f(1.0f, 0.0f, 1.5f);
    glVertex3f(-1.0f, 0.0f, 1.5f);
    // Top
    glVertex3f(-1.0f, 0.6f, -1.5f);
    glVertex3f(1.0f, 0.6f, -1.5f);
    glVertex3f(1.0f, 0.6f, 1.5f);
    glVertex3f(-1.0f, 0.6f, 1.5f);
    // Front
    glVertex3f(-1.0f, 0.0f, 1.5f);
    glVertex3f(1.0f, 0.0f, 1.5f);
    glVertex3f(1.0f, 0.6f, 1.5f);
    glVertex3f(-1.0f, 0.6f, 1.5f);
    // Back
    glVertex3f(-1.0f, 0.0f, -1.5f);
    glVertex3f(1.0f, 0.0f, -1.5f);
    glVertex3f(1.0f, 0.6f, -1.5f);
    glVertex3f(-1.0f, 0.6f, -1.5f);
    // Left side
    glVertex3f(-1.0f, 0.0f, -1.5f);
    glVertex3f(-1.0f, 0.0f, 1.5f);
    glVertex3f(-1.0f, 0.6f, 1.5f);
    glVertex3f(-1.0f, 0.6f, -1.5f);
    // Right side
    glVertex3f(1.0f, 0.0f, -1.5f);
    glVertex3f(1.0f, 0.0f, 1.5f);
    glVertex3f(1.0f, 0.6f, 1.5f);
    glVertex3f(1.0f, 0.6f, -1.5f);
    glEnd();
    // Cabin (roof)
    glColor3f(0.7f, 0.7f, 0.7f);
    glBegin(GL_QUADS);
    // Roof
    glVertex3f(-0.8f, 0.9f, -1.2f);
    glVertex3f(0.8f, 0.9f, -1.2f);
    glVertex3f(0.8f, 0.9f, 1.2f);
    glVertex3f(-0.8f, 0.9f, 1.2f);
    // Front face
    glVertex3f(-0.8f, 0.6f, 1.2f);
    glVertex3f(0.8f, 0.6f, 1.2f);
    glVertex3f(0.8f, 0.9f, 1.2f);
    glVertex3f(-0.8f, 0.9f, 1.2f);
    // Back face
    glVertex3f(-0.8f, 0.6f, -1.2f);
    glVertex3f(0.8f, 0.6f, -1.2f);
    glVertex3f(0.8f, 0.9f, -1.2f);
    glVertex3f(-0.8f, 0.9f, -1.2f);
    // Left face
    glVertex3f(-0.8f, 0.6f, -1.2f);
    glVertex3f(-0.8f, 0.6f, 1.2f);
    glVertex3f(-0.8f, 0.9f, 1.2f);
    glVertex3f(-0.8f, 0.9f, -1.2f);
    // Right face
    glVertex3f(0.8f, 0.6f, -1.2f);
    glVertex3f(0.8f, 0.6f, 1.2f);
    glVertex3f(0.8f, 0.9f, 1.2f);
    glVertex3f(0.8f, 0.9f, -1.2f);
    glEnd();

    // Wheels
    glColor3f(0.0f, 0.0f, 0.0f);
    float wheelRadius = 0.28f;
    float wheelY = wheelRadius;

    float wheelPos[4][2] = {
        {-1.0f, -1.7f}, {1.0f, -1.7f},
       {-1.0f, 1.7f}, {1.0f, 1.7f}
    };

    for (int i = 0; i < 4; ++i) {
        glPushMatrix();
        glTranslatef(wheelPos[i][0], wheelY, wheelPos[i][1]);
        glutSolidSphere(wheelRadius, 16, 16);
        glPopMatrix();
    }

    glPopMatrix();
}

void draw_person() {

    // Body
    glPushMatrix();
    glColor3f(0.8f, 0.5f, 0.3f);  // Shirt color
    glTranslatef(0.0f, 0.9f, 0.0f);
    glScalef(0.3f, 0.6f, 0.2f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Head
    glPushMatrix();
    glColor3f(1.0f, 0.8f, 0.6f);  // Skin color
    glTranslatef(0.0f, 1.4f, 0.0f);
    glutSolidSphere(0.15f, 20, 20);
    glPopMatrix();

    // Left Leg
    glPushMatrix();
    glColor3f(0.2f, 0.2f, 0.8f);  // Pants color
    glTranslatef(-0.07f, 0.4f, 0.0f);
    glScalef(0.1f, 0.5f, 0.1f);
    glutSolidCube(1.0);
    glPopMatrix();

    // Right Leg
    glPushMatrix();
    glColor3f(0.2f, 0.2f, 0.8f);
    glTranslatef(0.07f, 0.4f, 0.0f);
    glScalef(0.1f, 0.5f, 0.1f);
    glutSolidCube(1.0);
    glPopMatrix();
}


void restrict(){ // restrict movement within boundaries
    if (x > 300)    x = 300;
    else if (x < -300)    x = -300;
    if (y > 180)    y = 180;
    else if (y < 1.5)    y = 1.5;
    if (z > 300)    z = 300;
    else if (z < -300)    z = -300;
}

void mech_court()		//badminton court
{
    int k;
    glPushMatrix();
    glTranslatef(-70, 0.1, 20);
    glColor3f(0.5, 0.5, 0.5);
    glBegin(GL_QUADS);
    glVertex3f(0, 0, 0);
    glVertex3f(20, 0, 0);
    glVertex3f(20, 0, -40);
    glVertex3f(0, 0, -40);
    glEnd();
    glColor3f(0, 0, 0);
    for (k = 2;k < 18;k += 4)
    {
        glBegin(GL_QUADS);
        glVertex3f(k, 0, -0.1);
        glVertex3f(k, 4, -0.1);
        glVertex3f(k + 2, 4, -0.1);
        glVertex3f(k + 2, 0, -0.1);
        glEnd();
    }
    for (k = -2;k > -38;k -= 6)
    {
        glBegin(GL_QUADS);
        glVertex3f(0.1, 0, k);
        glVertex3f(0.1, 4, k);
        glVertex3f(0.1, 4, k - 2);
        glVertex3f(0.1, 0, k - 2);
        glEnd();
    }
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-50, 0.1, 20);
    for (k = -2;k > -38;k -= 6)
    {
        glBegin(GL_QUADS);
        glVertex3f(-0.1, 0, k);
        glVertex3f(-0.1, 4, k);
        glVertex3f(-0.1, 4, k - 2);
        glVertex3f(-0.1, 0, k - 2);
        glEnd();
    }
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-50, 0.1, -20);
    for (k = -2;k > -18;k -= 4)
    {
        glBegin(GL_QUADS);
        glVertex3f(k, 0, 0.1);
        glVertex3f(k, 4, 0.1);
        glVertex3f(k - 2, 4, 0.1);
        glVertex3f(k - 2, 0, 0.1);
        glEnd();
    }
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-65, 0.1, 15);
    glColor3f(0, 0, 1);
    glBegin(GL_QUADS);
    glVertex3f(0, 0.2, 0);
    glVertex3f(10, 0.2, 0);
    glVertex3f(10, 0.2, -30);
    glVertex3f(0, 0.2, -30);
    glEnd();
    glColor3f(1, 1, 1);
    glBegin(GL_LINE_LOOP);
    glVertex3f(1, 0.3, -1);
    glVertex3f(9, 0.3, -1);
    glVertex3f(9, 0.3, -13);
    glVertex3f(1, 0.3, -13);
    glEnd();
    glBegin(GL_LINE_LOOP);
    glVertex3f(1, 0.3, -15);
    glVertex3f(9, 0.3, -15);
    glVertex3f(9, 0.3, -29);
    glVertex3f(1, 0.3, -29);
    glEnd();
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-65, 0, 8);
    glColor3f(1, 1, 1);
    for (float i = 0;i < 0.8;i += 0.2)
        for (float j = 0;j < 9.8;j += 0.2)
        {
            glBegin(GL_LINE_LOOP);
            glVertex3f(j, i, 0);
            glVertex3f(j + 0.2, i, 0);
            glVertex3f(j + 0.2, i + 0.2, 0);
            glVertex3f(j, i + 0.2, 0);
            glEnd();
        }
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-65, 0, -8);
    glColor3f(1, 1, 1);
    for (float i = 0;i < 0.8;i += 0.2)
        for (float j = 0;j < 9.8;j += 0.2)
        {
            glBegin(GL_LINE_LOOP);
            glVertex3f(j, i, 0);
            glVertex3f(j + 0.2, i, 0);
            glVertex3f(j + 0.2, i + 0.2, 0);
            glVertex3f(j, i + 0.2, 0);
            glEnd();
        }
    glPopMatrix();
    for (int i = 0;i < 15;i += 5)
    {
        glPushMatrix();
        glTranslatef(-70, 6 + i, 20);
        glColor3f(1, 0.894, 0.709);
        glBegin(GL_QUADS);
        glVertex3f(0, 0, 0);
        glVertex3f(20, 0, 0);
        glVertex3f(20, 0, -4);
        glVertex3f(0, 0, -4);
        glEnd();
        glColor3f(1, 0.972, 0.862);
        glBegin(GL_QUADS);
        glVertex3f(4, 0, -4);
        glVertex3f(16, 0, -4);
        glVertex3f(16, 2, -4);
        glVertex3f(4, 2, -4);
        glEnd();
        glColor3f(0, 0, 0);
        for (k = 2;k < 18;k += 4)
        {
            glBegin(GL_QUADS);
            glVertex3f(k, 0, -0.1);
            glVertex3f(k, 4, -0.1);
            glVertex3f(k + 2, 4, -0.1);
            glVertex3f(k + 2, 0, -0.1);
            glEnd();
        }
        glPushMatrix();
        glTranslatef(20, 0, 0);
        glColor3f(1, 0.894, 0.709);
        glBegin(GL_QUADS);
        glVertex3f(0, 0, 0);
        glVertex3f(0, 0, -40);
        glVertex3f(-4, 0, -40);
        glVertex3f(-4, 0, 0);
        glEnd();
        glColor3f(1, 0.972, 0.862);
        glBegin(GL_QUADS);
        glVertex3f(-4, 0, -4);
        glVertex3f(-4, 0, -36);
        glVertex3f(-4, 2, -36);
        glVertex3f(-4, 2, -4);
        glEnd();
        glColor3f(0, 0, 0);
        for (k = -2;k > -38;k -= 6)
        {
            glBegin(GL_QUADS);
            glVertex3f(-0.1, 0, k);
            glVertex3f(-0.1, 4, k);
            glVertex3f(-0.1, 4, k - 2);
            glVertex3f(-0.1, 0, k - 2);
            glEnd();
        }
        glTranslatef(0, 0, -40);
        glColor3f(1, 0.894, 0.709);
        glBegin(GL_QUADS);
        glVertex3f(0, 0, 0);
        glVertex3f(-20, 0, 0);
        glVertex3f(-20, 0, 4);
        glVertex3f(0, 0, 4);
        glEnd();
        glColor3f(1, 0.972, 0.862);
        glBegin(GL_QUADS);
        glVertex3f(-4, 0, 4);
        glVertex3f(-16, 0, 4);
        glVertex3f(-16, 2, 4);
        glVertex3f(-4, 2, 4);
        glEnd();
        glColor3f(0, 0, 0);
        for (k = -2;k > -18;k -= 4)
        {
            glBegin(GL_QUADS);
            glVertex3f(k, 0, 0.1);
            glVertex3f(k, 4, 0.1);
            glVertex3f(k - 2, 4, 0.1);
            glVertex3f(k - 2, 0, 0.1);
            glEnd();
        }
        glPopMatrix();
        glColor3f(1, 0.894, 0.709);
        glBegin(GL_QUADS);
        glVertex3f(0, 0, 0);
        glVertex3f(0, 0, -40);
        glVertex3f(4, 0, -40);
        glVertex3f(4, 0, 0);
        glEnd();
        glColor3f(1, 0.972, 0.862);
        glBegin(GL_QUADS);
        glVertex3f(4, 0, -4);
        glVertex3f(4, 0, -36);
        glVertex3f(4, 2, -36);
        glVertex3f(4, 2, -4);
        glEnd();
        glColor3f(0, 0, 0);
        for (k = -2;k > -38;k -= 6)
        {
            glBegin(GL_QUADS);
            glVertex3f(0.1, 0, k);
            glVertex3f(0.1, 4, k);
            glVertex3f(0.1, 4, k - 2);
            glVertex3f(0.1, 0, k - 2);
            glEnd();
        }
        glPopMatrix();
    }
}

void disp_mba()	
{
    int k;
    glPushMatrix();
    glTranslatef(-70, 0.1, -30);
    glColor3f(0.5, 0.5, 0.5);
    glBegin(GL_QUADS);
    glVertex3f(0, 0, 0);
    glVertex3f(20, 0, 0);
    glVertex3f(20, 0, -40);
    glVertex3f(0, 0, -40);
    glEnd();
    glColor3f(0, 0, 0);
    for (k = 2;k < 18;k += 4)
    {
        glBegin(GL_QUADS);
        glVertex3f(k, 0, -0.1);
        glVertex3f(k, 4, -0.1);
        glVertex3f(k + 2, 4, -0.1);
        glVertex3f(k + 2, 0, -0.1);
        glEnd();
    }
    for (k = -2;k > -38;k -= 6)
    {
        glBegin(GL_QUADS);
        glVertex3f(0.1, 0, k);
        glVertex3f(0.1, 4, k);
        glVertex3f(0.1, 4, k - 2);
        glVertex3f(0.1, 0, k - 2);
        glEnd();
    }
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-50, 0.1, -30);
    for (k = -2;k > -38;k -= 6)
    {
        glBegin(GL_QUADS);
        glVertex3f(-0.1, 0, k);
        glVertex3f(-0.1, 4, k);
        glVertex3f(-0.1, 4, k - 2);
        glVertex3f(-0.1, 0, k - 2);
        glEnd();
    }
    glPopMatrix();
    glPushMatrix();
    glTranslatef(-50, 0.1, -70);
    for (k = -2;k > -18;k -= 4)
    {
        glBegin(GL_QUADS);
        glVertex3f(k, 0, 0.1);
        glVertex3f(k, 4, 0.1);
        glVertex3f(k - 2, 4, 0.1);
        glVertex3f(k - 2, 0, 0.1);
        glEnd();
    }
    glPopMatrix();
    for (int i = 0;i < 15;i += 5)
    {
        glPushMatrix();
        glTranslatef(-70, 6 + i, -30);
        glColor3f(1, 0.894, 0.709);
        glBegin(GL_QUADS);
        glVertex3f(0, 0, 0);
        glVertex3f(20, 0, 0);
        glVertex3f(20, 0, -4);
        glVertex3f(0, 0, -4);
        glEnd();
        glColor3f(1, 0.972, 0.862);
        glBegin(GL_QUADS);
        glVertex3f(4, 0, -4);
        glVertex3f(16, 0, -4);
        glVertex3f(16, 2, -4);
        glVertex3f(4, 2, -4);
        glEnd();
        glColor3f(0, 0, 0);
        for (k = 2;k < 18;k += 4)
        {
            glBegin(GL_QUADS);
            glVertex3f(k, 0, -0.1);
            glVertex3f(k, 4, -0.1);
            glVertex3f(k + 2, 4, -0.1);
            glVertex3f(k + 2, 0, -0.1);
            glEnd();
        }
        glPushMatrix();
        glTranslatef(20, 0, 0);
        glColor3f(1, 0.894, 0.709);
        glBegin(GL_QUADS);
        glVertex3f(0, 0, 0);
        glVertex3f(0, 0, -40);
        glVertex3f(-4, 0, -40);
        glVertex3f(-4, 0, 0);
        glEnd();
        glColor3f(1, 0.972, 0.862);
        glBegin(GL_QUADS);
        glVertex3f(-4, 0, -4);
        glVertex3f(-4, 0, -36);
        glVertex3f(-4, 2, -36);
        glVertex3f(-4, 2, -4);
        glEnd();
        glColor3f(0, 0, 0);
        for (k = -2;k > -38;k -= 6)
        {
            glBegin(GL_QUADS);
            glVertex3f(-0.1, 0, k);
            glVertex3f(-0.1, 4, k);
            glVertex3f(-0.1, 4, k - 2);
            glVertex3f(-0.1, 0, k - 2);
            glEnd();
        }
        glTranslatef(0, 0, -40);
        glColor3f(1, 0.894, 0.709);
        glBegin(GL_QUADS);
        glVertex3f(0, 0, 0);
        glVertex3f(-20, 0, 0);
        glVertex3f(-20, 0, 4);
        glVertex3f(0, 0, 4);
        glEnd();
        glColor3f(1, 0.972, 0.862);
        glBegin(GL_QUADS);
        glVertex3f(-4, 0, 4);
        glVertex3f(-16, 0, 4);
        glVertex3f(-16, 2, 4);
        glVertex3f(-4, 2, 4);
        glEnd();
        glColor3f(0, 0, 0);
        for (k = -2;k > -18;k -= 4)
        {
            glBegin(GL_QUADS);
            glVertex3f(k, 0, 0.1);
            glVertex3f(k, 4, 0.1);
            glVertex3f(k - 2, 4, 0.1);
            glVertex3f(k - 2, 0, 0.1);
            glEnd();
        }
        glPopMatrix();
        glColor3f(1, 0.894, 0.709);
        glBegin(GL_QUADS);
        glVertex3f(0, 0, 0);
        glVertex3f(0, 0, -40);
        glVertex3f(4, 0, -40);
        glVertex3f(4, 0, 0);
        glEnd();
        glColor3f(1, 0.972, 0.862);
        glBegin(GL_QUADS);
        glVertex3f(4, 0, -4);
        glVertex3f(4, 0, -36);
        glVertex3f(4, 2, -36);
        glVertex3f(4, 2, -4);
        glEnd();
        glColor3f(0, 0, 0);
        for (k = -2;k > -38;k -= 6)
        {
            glBegin(GL_QUADS);
            glVertex3f(0.1, 0, k);
            glVertex3f(0.1, 4, k);
            glVertex3f(0.1, 4, k - 2);
            glVertex3f(0.1, 0, k - 2);
            glEnd();
        }
        glPopMatrix();
    }

}

class temple		//construction of temple
{
    float stair[4][3];
    float room[8][3];
    float ceil[6][3];
public:
    temple()
    {
        stair[0][0] = 0;stair[0][1] = 0;stair[0][2] = 0;
        stair[1][0] = 12;stair[1][1] = 0;stair[1][2] = 0;
        stair[2][0] = 12;stair[2][1] = 0;stair[2][2] = -12;
        stair[3][0] = 0;stair[3][1] = 0;stair[3][2] = -12;

        room[0][0] = 0;room[0][1] = 0;room[0][2] = 0;
        room[1][0] = 0;room[1][1] = 6;room[1][2] = 0;
        room[2][0] = 0;room[2][1] = 6;room[2][2] = -7;
        room[3][0] = 0;room[3][1] = 0;room[3][2] = -7;
        room[4][0] = 7;room[4][1] = 0;room[4][2] = -7;
        room[5][0] = 7;room[5][1] = 6;room[5][2] = -7;
        room[6][0] = 7;room[6][1] = 6;room[6][2] = 0;
        room[7][0] = 7;room[7][1] = 0;room[7][2] = 0;

        ceil[0][0] = 0;ceil[0][1] = 6;ceil[0][2] = 4;
        ceil[1][0] = 3.5;ceil[1][1] = 9;ceil[1][2] = 4;
        ceil[2][0] = 7;ceil[2][1] = 6;ceil[2][2] = 4;
        ceil[3][0] = 7;ceil[3][1] = 6;ceil[3][2] = -9;
        ceil[4][0] = 3.5;ceil[4][1] = 9;ceil[4][2] = -9;
        ceil[5][0] = 0;ceil[5][1] = 6;ceil[5][2] = -9;
    }
    void disp_stair(int x, int y, int z)
    {
        glColor3f(1, 0.960, 0.933);
        glBegin(GL_QUADS);
        glVertex3f(stair[0][0] - x, stair[0][1] - y, stair[0][2] + z);
        glVertex3f(stair[1][0] + x, stair[1][1] - y, stair[1][2] + z);
        glVertex3f(stair[2][0] + x, stair[2][1] - y, stair[2][2] - z);
        glVertex3f(stair[3][0] - x, stair[3][1] - y, stair[3][2] - z);
        glEnd();
        glColor3f(0.933, 0.913, 0.8);
        glBegin(GL_QUADS);
        glVertex3f(stair[0][0] - x, stair[0][1] - y, stair[0][2] + z);
        glVertex3f(stair[0][0] - x, stair[0][1] - 1 - y, stair[0][2] + z);
        glVertex3f(stair[1][0] + x, stair[1][1] - 1 - y, stair[1][2] + z);
        glVertex3f(stair[1][0] + x, stair[1][1] - y, stair[1][2] + z);

        glVertex3f(stair[1][0] + x, stair[1][1] - y, stair[1][2] + z);
        glVertex3f(stair[1][0] + x, stair[1][1] - 1 - y, stair[1][2] + z);
        glVertex3f(stair[2][0] + x, stair[2][1] - 1 - y, stair[2][2] - z);
        glVertex3f(stair[2][0] + x, stair[2][1] - y, stair[2][2] - z);

        glVertex3f(stair[2][0] + x, stair[2][1] - y, stair[2][2] - z);
        glVertex3f(stair[3][0] - x, stair[3][1] - y, stair[3][2] - z);
        glVertex3f(stair[3][0] - x, stair[3][1] - 1 - y, stair[3][2] - z);
        glVertex3f(stair[2][0] + x, stair[2][1] - 1 - y, stair[2][2] - z);

        glVertex3f(stair[3][0] - x, stair[3][1] - y, stair[3][2] - z);
        glVertex3f(stair[0][0] - x, stair[0][1] - y, stair[0][2] + z);
        glVertex3f(stair[0][0] - x, stair[0][1] - 1 - y, stair[0][2] + z);
        glVertex3f(stair[3][0] - x, stair[3][1] - 1 - y, stair[3][2] - z);
        glEnd();
    }

    void disp_room()
    {
        glColor3f(0.803, 0.803, 0.756);
        glBegin(GL_QUADS);
        glVertex3fv(room[0]);
        glVertex3fv(room[1]);
        glVertex3fv(room[2]);
        glVertex3fv(room[3]);
        glVertex3fv(room[3]);
        glVertex3fv(room[2]);
        glVertex3fv(room[5]);
        glVertex3fv(room[4]);
        glVertex3fv(room[4]);
        glVertex3fv(room[5]);
        glVertex3fv(room[6]);
        glVertex3fv(room[7]);
        glVertex3fv(room[1]);
        glVertex3fv(room[2]);
        glVertex3fv(room[5]);
        glVertex3fv(room[6]);
        glVertex3fv(room[0]);
        glVertex3f(room[0][0] + 1, room[0][1], room[0][2]);
        glVertex3f(room[1][0] + 1, room[1][1], room[1][2]);
        glVertex3fv(room[1]);
        glVertex3fv(room[7]);
        glVertex3f(room[7][0] - 1, room[7][1], room[7][2]);
        glVertex3f(room[6][0] - 1, room[6][1], room[6][2]);
        glVertex3fv(room[6]);
        glEnd();
    }
    void disp_ceil()
    {
        glColor3f(1, 0.843, 0);
        glBegin(GL_TRIANGLES);
        glVertex3fv(ceil[2]);
        glVertex3fv(ceil[1]);
        glVertex3fv(ceil[0]);
        glVertex3fv(ceil[3]);
        glVertex3fv(ceil[4]);
        glVertex3fv(ceil[5]);
        glEnd();
        glColor3f(0.933, 0.866, 0.509);
        glBegin(GL_POLYGON);
        glVertex3fv(ceil[2]);
        glVertex3fv(ceil[1]);
        glVertex3fv(ceil[4]);
        glVertex3fv(ceil[3]);
        glEnd();
        glBegin(GL_POLYGON);
        glVertex3fv(ceil[0]);
        glVertex3fv(ceil[1]);
        glVertex3fv(ceil[4]);
        glVertex3fv(ceil[5]);
        glEnd();
    }
    void draw_pil()
    {
        GLUquadricObj* quadratic;
        quadratic = gluNewQuadric();
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glColor3f(0.933, 0.866, 0.509);
        gluCylinder(quadratic, 0.5, 0.5, 6.0f, 32, 32);
        glPopMatrix();
    }
    void draw_mesh()
    {
        glColor3f(1, 0.843, 0);
        for (float i = 0;i < 0.9;i += 0.2)
            for (float j = 0;j < 6;j += 0.2)
            {
                glBegin(GL_LINE_LOOP);
                glVertex3f(i, j, 0);
                glVertex3f(i + 0.2, j, 0);
                glVertex3f(i + 0.2, j + 0.2, 0);
                glVertex3f(i, j + 0.2, 0);
                glEnd();
            }
    }
    void disp_temple()
    {
        disp_stair(0, 0, 0);
        glPushMatrix();
        disp_stair(2, 1, 2);
        disp_stair(4, 2, 4);
        glPopMatrix();
        glPushMatrix();
        glTranslatef(4, 0, -9);
        glRotatef(-90, 0, 1, 0);
        disp_room();
        disp_ceil();
        glPushMatrix();
        glTranslatef(0.4, 0, 2.5);
        draw_pil();
        glTranslatef(6.2, 0, 0);
        draw_pil();
        glPopMatrix();
        glPushMatrix();
        glTranslatef(1.5, 0, -3);
        draw_idol();
        glPopMatrix();
        glPushMatrix();
        glTranslatef(1, 0, 0);
        draw_mesh();
        glTranslatef(4, 0, 0);
        draw_mesh();
        glPopMatrix();
        glPopMatrix();
    }
}temp;

class building			//construction of the block buildings
{
    float structure[8][3];
public:
    building(float a, float b, float c)
    {
        structure[0][0] = 0;structure[0][1] = 0;structure[0][2] = 0;
        structure[1][0] = a;structure[1][1] = 0;structure[1][2] = 0;
        structure[2][0] = a;structure[2][1] = b;structure[2][2] = 0;
        structure[3][0] = 0;structure[3][1] = b;structure[3][2] = 0;
        structure[4][0] = 0;structure[4][1] = 0;structure[4][2] = c;
        structure[5][0] = a;structure[5][1] = 0;structure[5][2] = c;
        structure[6][0] = a;structure[6][1] = b;structure[6][2] = c;
        structure[7][0] = 0;structure[7][1] = b;structure[7][2] = c;
    }
    void disp_build(const char* text, char side = '\0')
    {
        float door[3];
        glColor3f(1, 0.980, 0.980);
        glBegin(GL_QUADS);
        glVertex3fv(structure[0]);
        glVertex3fv(structure[1]);
        glVertex3fv(structure[2]);
        glVertex3fv(structure[3]);
        glEnd();
        glBegin(GL_QUADS);
        glVertex3fv(structure[0]);
        glVertex3fv(structure[4]);
        glVertex3fv(structure[7]);
        glVertex3fv(structure[3]);
        glEnd();
        glBegin(GL_QUADS);
        glVertex3fv(structure[4]);
        glVertex3fv(structure[5]);
        glVertex3fv(structure[6]);
        glVertex3fv(structure[7]);
        glEnd();
        glBegin(GL_QUADS);
        glVertex3fv(structure[1]);
        glVertex3fv(structure[2]);
        glVertex3fv(structure[6]);
        glVertex3fv(structure[5]);
        glEnd();

        if (structure[1][0] > (-1 * structure[4][2]))
        {
            for (float i = 10; i < structure[2][1]; i += 10)
            {
                glPushMatrix();
                glTranslatef(0, i, 0);
                for (float j = 5; j < structure[1][0]; j += 15)
                {
                    glColor3f(0, 0, 0);
                    glBegin(GL_POLYGON);
                    glVertex3f(j, 0, 0.1);
                    glVertex3f(j + 5, 0, 0.1);
                    glVertex3f(j + 5, 5, 0.1);
                    glVertex3f(j, 5, 0.1);
                    glEnd();
                    glBegin(GL_POLYGON);
                    glVertex3f(j, 0, structure[4][2] - 0.1);
                    glVertex3f(j + 5, 0, structure[4][2] - 0.1);
                    glVertex3f(j + 5, 5, structure[4][2] - 0.1);
                    glVertex3f(j, 5, structure[4][2] - 0.1);
                    glEnd();
                }
                for (float j = 0;j < structure[1][0];j += 15)
                {
                    glColor3f(1, 0, 0);
                    glBegin(GL_POLYGON);
                    glVertex3f(j, -10, 0.1);
                    glVertex3f(j + 2, -10, 0.1);
                    glVertex3f(j + 2, 10, 0.1);
                    glVertex3f(j, 10, 0.1);
                    glEnd();
                    glBegin(GL_POLYGON);
                    glVertex3f(j, -10, structure[4][2] - 0.1);
                    glVertex3f(j + 2, -10, structure[4][2] - 0.1);
                    glVertex3f(j + 2, 10, structure[4][2] - 0.1);
                    glVertex3f(j, 10, structure[4][2] - 0.1);
                    glEnd();
                }
                glPopMatrix();
            }
            glColor3f(0, 0, 0);
            door[0] = (structure[1][0] / 2);
            glBegin(GL_POLYGON);
            glVertex3f(door[0] - 4, 0, 0.2);
            glVertex3f(door[0] + 4, 0, 0.2);
            glVertex3f(door[0] + 4, 7, 0.2);
            glVertex3f(door[0] - 4, 7, 0.2);
            glEnd();
            glPushMatrix();
            glTranslatef(10, 0, 3);
            draw_board();
            glPushMatrix();
            glTranslatef(1, 2, 0.1);
            glScalef(.01, .01, .01);
            glLineWidth(2);
            glColor3f(0, 0, 0);
            for (int c = 0; text[c] != 0; ++c)
                glutStrokeCharacter(GLUT_STROKE_ROMAN, text[c]);
            glPopMatrix();
            glPopMatrix();
        }
        else
        {
            for (float i = 10; i < structure[2][1]; i += 10)
            {
                glPushMatrix();
                glTranslatef(0, i, 0);
                for (float j = -5; j > structure[4][2]; j -= 15)
                {
                    glColor3f(0, 0, 0);
                    glBegin(GL_POLYGON);
                    glVertex3f(-0.1, 0, j);
                    glVertex3f(-0.1, 0, j - 5);
                    glVertex3f(-0.1, 5, j - 5);
                    glVertex3f(-0.1, 5, j);
                    glEnd();
                    glBegin(GL_POLYGON);
                    glVertex3f(structure[1][0] + 0.1, 0, j);
                    glVertex3f(structure[1][0] + 0.1, 0, j - 5);
                    glVertex3f(structure[1][0] + 0.1, 5, j - 5);
                    glVertex3f(structure[1][0] + 0.1, 5, j);
                    glEnd();
                }
                for (float j = 0;j > structure[4][2];j -= 15)
                {
                    glColor3f(1, 0, 0);
                    glBegin(GL_POLYGON);
                    glVertex3f(-0.1, -10, j);
                    glVertex3f(-0.1, -10, j - 2);
                    glVertex3f(-0.1, 10, j - 2);
                    glVertex3f(-0.1, 10, j);
                    glEnd();
                    glBegin(GL_POLYGON);
                    glVertex3f(structure[1][0] + 0.1, -10, j);
                    glVertex3f(structure[1][0] + 0.1, -10, j - 2);
                    glVertex3f(structure[1][0] + 0.1, 10, j - 2);
                    glVertex3f(structure[1][0] + 0.1, 10, j);
                    glEnd();
                }
                glPopMatrix();
            }
            door[2] = (structure[4][2] / 2);
            door[0] = structure[1][0];
            glColor3f(0, 0, 0);
            if (side == 'r')
            {
                glBegin(GL_POLYGON);
                glVertex3f(door[0] + 0.2, 0, door[2] - 4);
                glVertex3f(door[0] + 0.2, 0, door[2] + 4);
                glVertex3f(door[0] + 0.2, 7, door[2] + 4);
                glVertex3f(door[0] + 0.2, 7, door[2] - 4);
                glEnd();
                glPushMatrix();
                glTranslatef(door[0] + 3, 0, -2);
                glRotatef(90, 0, 1, 0);
                draw_board();
                glPushMatrix();
                glTranslatef(1, 2, 0.1);
                glScalef(.01, .01, .01);
                glLineWidth(2);
                glColor3f(0, 0, 0);
                for (int c = 0; text[c] != 0; ++c)
                    glutStrokeCharacter(GLUT_STROKE_ROMAN, text[c]);
                glPopMatrix();
                glPopMatrix();
            }
            else if (side == 'l')
            {
                glBegin(GL_POLYGON);
                glVertex3f(-0.2, 0, door[2] - 4);
                glVertex3f(-0.2, 0, door[2] + 4);
                glVertex3f(-0.2, 7, door[2] + 4);
                glVertex3f(-0.2, 7, door[2] - 4);
                glEnd();
                glPushMatrix();
                glTranslatef(-3, 0, -10);
                glRotatef(-90, 0, 1, 0);
                draw_board();
                glPushMatrix();
                glTranslatef(1, 2, 0.1);
                glScalef(.01, .01, .01);
                glLineWidth(2);
                glColor3f(0, 0, 0);
                for (int c = 0; text[c] != 0; ++c)
                    glutStrokeCharacter(GLUT_STROKE_ROMAN, text[c]);
                glPopMatrix();
                glPopMatrix();
            }
        }
        glPushMatrix();
        glTranslatef(0, 10, 0);
        glColor3f(0, 0, 1);
        for (int i = 0;i < structure[2][1] - 5;i += 5)
        {
            glBegin(GL_LINES);
            glVertex3f(structure[0][0], i, structure[0][2] + 0.1);
            glVertex3f(structure[1][0], i, structure[0][2] + 0.1);
            glVertex3f(structure[0][0] - 0.1, i, structure[0][2]);
            glVertex3f(structure[0][0] - 0.1, i, structure[4][2]);
            glVertex3f(structure[4][0], i, structure[4][2] - 0.1);
            glVertex3f(structure[5][0], i, structure[4][2] - 0.1);
            glVertex3f(structure[5][0] + 0.1, i, structure[5][2]);
            glVertex3f(structure[1][0] + 0.1, i, structure[1][2]);

            glEnd();
        }
        glPopMatrix();
    }
};
building canteen(20, 30, -30);
building mech(20, 40, -40);
building mba(20, 40, -40);
building admin(40, 30, -20);
building ec(40, 30, -30);
building cs(40, 40, -40);
building hospital(20, 20, -40);
building library(80, 30, -20);     
building auditorium(40, 40, -40);  
building hostelA(30, 40, -30);      
building hostelB(30, 40, -30);     
building hostelC(30, 40, -30);      
building hostelD(30, 40, -30);
building research(20, 40, -40);     // width, height, depth
building ccos(20, 20, -30);

//the basket loop of the basketball post
void loop(float x, float y, float z)
{
    float xx, zz, d;
    glColor3f(1, 0, 0);
    glPointSize(2);
    glBegin(GL_POINTS);
    for (int i = 0;i < 360;i++)
    {
        d = i * (180 / 3.14);
        xx = cos(d) + x;
        zz = sin(d) + z;
        glVertex3f(xx, y, zz);
    }
    glEnd();
}

//construction of basketball court
class bball
{
    float bordr[4][3];
    float bskt[8][3];
public:
    bball()
    {
        bordr[0][0] = 0;bordr[0][1] = 0;bordr[0][2] = 0;
        bordr[1][0] = 20;bordr[1][1] = 0;bordr[1][2] = 0;
        bordr[2][0] = 20;bordr[2][1] = 0;bordr[2][2] = -20;
        bordr[3][0] = 0;bordr[3][1] = 0;bordr[3][2] = -20;
        bskt[0][0] = 14;bskt[0][1] = 4.5;bskt[0][2] = -0.1;
        bskt[1][0] = 16;bskt[1][1] = 4.5;bskt[1][2] = -0.1;
        bskt[2][0] = 16;bskt[2][1] = 6.5;bskt[2][2] = -0.1;
        bskt[3][0] = 14;bskt[3][1] = 6.5;bskt[3][2] = -0.1;
        bskt[4][0] = 14, bskt[4][1] = 4.5;bskt[4][2] = -19.9;
        bskt[5][0] = 16;bskt[5][1] = 4.5;bskt[5][2] = -19.9;
        bskt[6][0] = 16;bskt[6][1] = 6.5;bskt[6][2] = -19.9;
        bskt[7][0] = 14;bskt[7][1] = 6.5;bskt[7][2] = -19.9;
    }
    void disp_court()
    {
        glPushMatrix();
        glTranslatef(0, 0.1, 0);
        glColor3f(0.745, 0.745, 0.745);
        glBegin(GL_QUADS);
        glVertex3fv(bordr[0]);
        glVertex3fv(bordr[1]);
        glVertex3fv(bordr[2]);
        glVertex3fv(bordr[3]);
        glEnd();
        glColor3f(1, 0.270, 0);
        GLUquadricObj* quadratic;
        quadratic = gluNewQuadric();
        GLUquadricObj* quadratic1;
        quadratic1 = gluNewQuadric();
        glPushMatrix();
        glTranslatef(15, 0, 0);
        loop(0, 5, -1);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glColor3f(0.698, 0.133, 0.133);
        gluCylinder(quadratic, 0.1, 0.1, 5.0f, 32, 32);
        glPopMatrix();
        glPopMatrix();
        glPushMatrix();
        glTranslatef(15, 0, -20);
        loop(0, 5, 1);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        glColor3f(0.698, 0.133, 0.133);
        gluCylinder(quadratic1, 0.1, 0.1, 5.0f, 32, 32);
        glPopMatrix();
        glPopMatrix();
        glColor3f(0.745, 0.745, 0.745);
        glBegin(GL_QUADS);
        for (int i = 0;i < 8;i++)
            glVertex3fv(bskt[i]);
        glEnd();
        glPopMatrix();
    }
};
bball crt1;

// construction of the football field
class ground
{
    float bordr[4][3];
public:
    ground()
    {
        bordr[0][0] = 0;    bordr[0][1] = 0;    bordr[0][2] = 0;
        bordr[1][0] = 40;   bordr[1][1] = 0;    bordr[1][2] = 0;
        bordr[2][0] = 40;   bordr[2][1] = 0;    bordr[2][2] = -40;
        bordr[3][0] = 0;    bordr[3][1] = 0;    bordr[3][2] = -40;
    }

    void ground_disp()
    {
        // Draw the football field (Top surface)
        glColor3f(0.0f, 0.4f, 0.0f); // dark green
        glBegin(GL_QUADS);
        glVertex3f(bordr[0][0], bordr[0][1] + 0.01f, bordr[0][2]);
        glVertex3f(bordr[1][0], bordr[1][1] + 0.01f, bordr[1][2]);
        glVertex3f(bordr[2][0], bordr[2][1] + 0.01f, bordr[2][2]);
        glVertex3f(bordr[3][0], bordr[3][1] + 0.01f, bordr[3][2]);
        glEnd();

        // Goal Post 1
        glPushMatrix();
        {
            glTranslatef(16.5f, 0.0f, -7.0f);
            glColor3f(1.0f, 1.0f, 1.0f); 
            glLineWidth(5.0f);
            glBegin(GL_LINE_LOOP);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, 4.0f, 0.0f);
            glVertex3f(4.0f, 4.0f, 0.0f);
            glVertex3f(4.0f, 0.0f, 0.0f);
            glEnd();
        }
        glPopMatrix();

        // Goal Post 2
        glPushMatrix();
        {
            glTranslatef(16.5f, 0.0f, -33.0f);
            glColor3f(1.0f, 1.0f, 1.0f);
            glLineWidth(5.0f);
            glBegin(GL_LINE_LOOP);
            glVertex3f(0.0f, 0.0f, 0.0f);
            glVertex3f(0.0f, 4.0f, 0.0f);
            glVertex3f(4.0f, 4.0f, 0.0f);
            glVertex3f(4.0f, 0.0f, 0.0f);
            glEnd();
        }
        glPopMatrix();
    }
} fball;


void badminton_ground_disp() {
    glPushMatrix();
    glTranslatef(-55, 0.1, -100);  

    // Court dimensions
    float court_width = 20.0f;     
    float court_length = 40.0f;    

    // Court floor
    glColor3f(0.5, 0.5, 0.5); 
    glBegin(GL_QUADS);
    glVertex3f(0, 0, 0);
    glVertex3f(court_width, 0, 0);
    glVertex3f(court_width, 0, -court_length);
    glVertex3f(0, 0, -court_length);
    glEnd();

    // Draw court boundary
    glColor3f(1, 1, 1);  
    glLineWidth(2.0);
    glBegin(GL_LINE_LOOP);
    glVertex3f(0, 0.01, 0);
    glVertex3f(court_width, 0.01, 0);
    glVertex3f(court_width, 0.01, -court_length);
    glVertex3f(0, 0.01, -court_length);
    glEnd();

    // Center line (divides the service courts)
    glBegin(GL_LINES);
    glVertex3f(court_width / 2, 0.01, 0);
    glVertex3f(court_width / 2, 0.01, -court_length);
    glEnd();

    // Short service line (near the net)
    glBegin(GL_LINES);
    glVertex3f(0, 0.01, -5);
    glVertex3f(court_width, 0.01, -5);
    glEnd();

    // Long service line for doubles (at the back)
    glBegin(GL_LINES);
    glVertex3f(0, 0.01, -39);
    glVertex3f(court_width, 0.01, -39);
    glEnd();

    // Side lines for singles (inner boundary)
    glBegin(GL_LINES);
    glVertex3f(2, 0.01, 0);
    glVertex3f(2, 0.01, -court_length);
    glVertex3f(court_width - 2, 0.01, 0);
    glVertex3f(court_width - 2, 0.01, -court_length);
    glEnd();

    // Net Area
    glPushMatrix();
    glTranslatef(0, 0, -court_length / 2);  

    // Net base
    glColor3f(0, 0, 1);  
    glBegin(GL_QUADS);
    glVertex3f(0, 0, -0.05);
    glVertex3f(court_width, 0, -0.05);
    glVertex3f(court_width, 0, 0.05);
    glVertex3f(0, 0, 0.05);
    glEnd();

    // Net mesh
    glColor3f(1, 1, 1);  
    float net_height = 2.0f;  
    float net_gap = 0.5f;    

    for (float h = 0; h <= net_height; h += net_gap) {
        glBegin(GL_LINES);
        glVertex3f(0, h, 0);
        glVertex3f(court_width, h, 0);
        glEnd();
    }
    for (float w = 0; w <= court_width; w += net_gap) {
        glBegin(GL_LINES);
        glVertex3f(w, 0, 0);
        glVertex3f(w, net_height, 0);
        glEnd();
    }
    glPopMatrix();
    glPopMatrix();
}

void badminton_ground_disp2() {
    glPushMatrix();
    glTranslatef(-80, 0.1, -100); 

    // Court dimensions
    float court_width = 20.0f;    
    float court_length = 40.0f;   

    // Court floor
    glColor3f(0.5, 0.5, 0.5);  
    glBegin(GL_QUADS);
    glVertex3f(0, 0, 0);
    glVertex3f(court_width, 0, 0);
    glVertex3f(court_width, 0, -court_length);
    glVertex3f(0, 0, -court_length);
    glEnd();

    // Draw court boundary
    glColor3f(1, 1, 1);  
    glLineWidth(2.0);
    glBegin(GL_LINE_LOOP);
    glVertex3f(0, 0.01, 0);
    glVertex3f(court_width, 0.01, 0);
    glVertex3f(court_width, 0.01, -court_length);
    glVertex3f(0, 0.01, -court_length);
    glEnd();

    // Center line (divides the service courts)
    glBegin(GL_LINES);
    glVertex3f(court_width / 2, 0.01, 0);
    glVertex3f(court_width / 2, 0.01, -court_length);
    glEnd();

    // Short service line (near the net)
    glBegin(GL_LINES);
    glVertex3f(0, 0.01, -5);
    glVertex3f(court_width, 0.01, -5);
    glEnd();

    // Long service line for doubles (at the back)
    glBegin(GL_LINES);
    glVertex3f(0, 0.01, -39);
    glVertex3f(court_width, 0.01, -39);
    glEnd();

    // Side lines for singles (inner boundary)
    glBegin(GL_LINES);
    glVertex3f(2, 0.01, 0);
    glVertex3f(2, 0.01, -court_length);
    glVertex3f(court_width - 2, 0.01, 0);
    glVertex3f(court_width - 2, 0.01, -court_length);
    glEnd();

    // Net Area
    glPushMatrix();
    glTranslatef(0, 0, -court_length / 2);  

    // Net base
    glColor3f(0, 0, 1);  
    glBegin(GL_QUADS);
    glVertex3f(0, 0, -0.05);
    glVertex3f(court_width, 0, -0.05);
    glVertex3f(court_width, 0, 0.05);
    glVertex3f(0, 0, 0.05);
    glEnd();

    // Net mesh
    glColor3f(1, 1, 1); 
    float net_height = 2.0f;  
    float net_gap = 0.5f;     

    for (float h = 0; h <= net_height; h += net_gap) {
        glBegin(GL_LINES);
        glVertex3f(0, h, 0);
        glVertex3f(court_width, h, 0);
        glEnd();
    }
    for (float w = 0; w <= court_width; w += net_gap) {
        glBegin(GL_LINES);
        glVertex3f(w, 0, 0);
        glVertex3f(w, net_height, 0);
        glEnd();
    }
    glPopMatrix();
    glPopMatrix();
}

void changeSize(int w, int h)
{
    float ratio = ((float)w) / ((float)h); // window aspect ratio
    glMatrixMode(GL_PROJECTION); // projection matrix is active
    glLoadIdentity(); // reset the projection
    gluPerspective(45.0, ratio, 0.1, 250.0); // perspective transformation
    glMatrixMode(GL_MODELVIEW); // return to modelview mode
    glViewport(0, 0, w, h); // set viewport (drawing area) to entire window
}

void updatePeopleAndVehicles() {

    for (int i = 0; i < numPeople; ++i) {
        people[i].z += people[i].speed;
        if (people[i].z > 300) people[i].z = -300;
    }
}
void update(void)
{
    if (deltaMove) { // update camera position
        x += deltaMove * lx * 0.38;
        z += deltaMove * lz * 0.38;
    }
    if (vertMove == 1)    y += 0.1;
    if (vertMove == -1) y -= 0.1;
    restrict();
    glutPostRedisplay(); // redisplay everything

    // Move People
    for (int i = 0; i < numPeople; i++) {
        people[i].z += people[i].speed * 0.4f;
        if (people[i].z > 90) people[i].z = -90;  // reset to start after reaching end
    }
    for (int i = 0; i < numCars; ++i) {
        float& x = cars[i].x;
        float& z = cars[i].z;
        float speed = cars[i].speed;

        switch (cars[i].pathIndex) {
        case ROAD1_DOWN: // Road 1: Top → Bottom (x=-40 to -20)
            z -= speed * 0.5f;

            // Safety: Clamp x to road bounds
            if (x < ROAD1_X_MIN) x = ROAD1_X_MIN;
            if (x > ROAD1_X_MAX) x = ROAD1_X_MAX;

            // Transition to Road 2
            if (z <= ROAD1_Z_END) {
                z = ROAD2_Z_MIN;
                x = ROAD2_X_START;
                cars[i].pathIndex = ROAD2_RIGHT;
            }
            break;

        case ROAD2_RIGHT: // Road 2: Left → Right (z=70 to 80)
            x += speed * 0.5f;

            // Safety: Clamp z to road bounds
            if (z < ROAD2_Z_MIN) z = ROAD2_Z_MIN;
            if (z > ROAD2_Z_MAX) z = ROAD2_Z_MAX;

            // Transition to Road 3
            if (x >= ROAD2_X_END) {
                x = ROAD3_X_MIN;
                z = ROAD3_Z_START;
                cars[i].pathIndex = ROAD3_UP;
            }
            break;

        case ROAD3_UP: // Road 3: Bottom → Top (x=35 to 45)
            z += speed * 0.5f;

            // Safety: Clamp x to road bounds
            if (x < ROAD3_X_MIN) x = ROAD3_X_MIN;
            if (x > ROAD3_X_MAX) x = ROAD3_X_MAX;

            // Transition to Road 4
            if (z >= ROAD3_Z_END) {
                z = ROAD4_Z_MIN;
                x = ROAD4_X_START;
                cars[i].pathIndex = ROAD4_LEFT;
            }
            break;

        case ROAD4_LEFT: // Road 4: Right → Left (z=-95 to -80)
            x -= speed * 0.5f;

            // Safety: Clamp z to road bounds
            if (z < ROAD4_Z_MIN) z = ROAD4_Z_MIN;
            if (z > ROAD4_Z_MAX) z = ROAD4_Z_MAX;

            // Transition to Road 5
            if (x <= ROAD4_X_END) {
                x = ROAD5_X_MIN;
                z = ROAD5_Z_START;
                cars[i].pathIndex = ROAD5_DOWN;
            }
            break;

        case ROAD5_DOWN: // Road 5: Top → Bottom (x=-95 to -80)
            z -= speed * 0.5f;

            // Safety: Clamp x to road bounds
            if (x < ROAD5_X_MIN) x = ROAD5_X_MIN;
            if (x > ROAD5_X_MAX) x = ROAD5_X_MAX;

            // Loop back to Road 1
            if (z <= ROAD5_Z_END) {
                z = ROAD1_Z_START;
                x = ROAD1_X_MIN;
                cars[i].pathIndex = ROAD1_DOWN;
            }
            break;
        }
    }

};

void disp_roads()			//display the roads in the campus
{
    glColor3f(0.411, 0.411, 0.411);
    glBegin(GL_QUADS);
    glVertex3f(-40, 0.1, 90);
    glVertex3f(-40, 0.1, -80);
    glVertex3f(-20, 0.1, -80);
    glVertex3f(-20, 0.1, 90);
    glEnd();

    glBegin(GL_QUADS);
    glVertex3f(-20, 0.1, 70);
    glVertex3f(40, 0.1, 70);
    glVertex3f(40, 0.1, 80);
    glVertex3f(-20, 0.1, 80);
    glEnd();
    glBegin(GL_QUADS);
    glVertex3f(35, 0.1, 80);
    glVertex3f(35, 0.1, -90);
    glVertex3f(45, 0.1, -90);
    glVertex3f(45, 0.1, 80);
    glEnd();

    glBegin(GL_QUADS);
    glVertex3f(120, 0.1, -95);
    glVertex3f(120, 0.1, -80);
    glVertex3f(-95, 0.1, -80);
    glVertex3f(-95, 0.1, -95);
    glEnd();

    glBegin(GL_QUADS);
    glVertex3f(-95, 0.1, 80);
    glVertex3f(-95, 0.1, -80);
    glVertex3f(-80, 0.1, -80);
    glVertex3f(-80, 0.1, 80);
    glEnd();
}

void trees()			//draw a tree
{
    GLUquadricObj* quadratic;
    GLUquadricObj* quadratic1;
    quadratic1 = gluNewQuadric();
    quadratic = gluNewQuadric();
    glPushMatrix();
    glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
    glColor3f(0.721, 0.525, 0.043);
    gluCylinder(quadratic, 1, 1, 10.0f, 32, 32);
    glPopMatrix();
    glTranslatef(0, 2, 0);
    glPushMatrix();
    float k = 0;
    for (int i = 0, j = 0;i < 3;i++, j += 0.5, k += 0.15)
    {
        glTranslatef(0, 1.8, 0);
        glColor3f(0.133 + k, 0.545 + k, 0.133 - k);
        glPushMatrix();
        glRotatef(-90.0f, 1.0f, 0.0f, 0.0f);
        gluCylinder(quadratic1, 4 - j, 0, 4.0f, 32, 32);
        glPopMatrix();
    }
    glPopMatrix();
}

void draw_arch(const char* text)		//draw the arch
{
    glColor3f(0, 0, 1);
    glPushMatrix();
    glTranslatef(0, 3.5, 0);
    glScalef(4, 7, 2);
    glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(16, 3.5, 0);
    glScalef(4, 7, 2);
    glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(8, 9, 0);
    glScalef(20, 4, 2);
    glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
    glTranslatef(5, 8, 1.1);
    glScalef(.02, .02, .02);
    glLineWidth(4.5);
    glColor3f(1, 1, 1);
    for (int c = 0; text[c] != 0; ++c)
        glutStrokeCharacter(GLUT_STROKE_ROMAN, text[c]);
    glPopMatrix();
}

void drawAllPeople() {
    for (int i = 0; i < numPeople; ++i) {
        glPushMatrix();
        glTranslatef(people[i].x, people[i].y, people[i].z);
        draw_person();
        glPopMatrix();
    }
}
void idle() {
    updatePeopleAndVehicles(); // add this
    glutPostRedisplay();
};
void draw_cars() {
    for (int i = 0; i < numCars; ++i) {
        draw_vehicle(
            cars[i].x, 0.3f, cars[i].z,  // Position (y=0.3f for height)
            1.0f,                        // Scale
            cars[i].r, cars[i].g, cars[i].b  // Color
        );
    }
}


void display()
{
    glClearColor(0.7, 0.7, 1, 0); //sky color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glLoadIdentity();
    gluLookAt(
        x, y, z,
        x + lx, y, z + lz,
        0.0, 1.0, 0.0);
    printf("\nz=%f\tx=%f\n", z, x);
    glColor3f(0, 1, 0); //green color
    //size of base green ground
    glBegin(GL_QUADS);
    glVertex3f(-300, 0, 300);
    glVertex3f(300, 0, 300);
    glVertex3f(300, 0, 20);
    glVertex3f(-300, 0, 20);

    glVertex3f(-300, 0, 20);
    glVertex3f(-15, 0, 20);
    glVertex3f(-15, 0, -300);
    glVertex3f(-300, 0, -300);

    glVertex3f(-15, 0, -300);
    glVertex3f(300, 0, -300);
    glVertex3f(300, 0, -20);
    glVertex3f(-15, 0, -20);

    glVertex3f(25, 0, -20);
    glVertex3f(300, 0, -20);
    glVertex3f(300, 0, 20);
    glVertex3f(25, 0, 20);
    glEnd();
    draw_map();

    //Roads
    disp_roads();
    draw_cars();

    //Display trees
    for (int i = 50; i <= 130; i += 10)  //in front of library
    {
        glPushMatrix();
        glTranslatef(i, 0, 80);
        trees();
        glPopMatrix();
    }

    for (int i = -10; i <= 130; i += 10)  //in front of library (longer one)
    {
        glPushMatrix();
        glTranslatef(i, 0, 85);
        trees();
        glPopMatrix();
    }

    for (int i = -10; i <= 20; i += 10)  //between cs block and admin
    {
        glPushMatrix();
        glTranslatef(i, 0, -20);
        trees();
        glPopMatrix();
    }

    for (int i = 50; i <= 130; i += 10)  //between cs block and admin
    {
        glPushMatrix();
        glTranslatef(i, 0, 30);
        trees();
        glPopMatrix();
    }

    for (int i = -140; i <= -110; i += 10)  //between research block and football ground
    {
        glPushMatrix();
        glTranslatef(i, 0, -80);
        trees();
        glPopMatrix();
    }

    for (int i = -140; i <= -110; i += 10)  //between research block and football ground
    {
        glPushMatrix();
        glTranslatef(i, 0, -90);
        trees();
        glPopMatrix();
    }

    for (int i = -150; i <= -80; i += 10)  //between research block and football ground
    {
        glPushMatrix();
        glTranslatef(-150, 0, i);
        trees();
        glPopMatrix();
    }

    for (int i = -150; i <= -80; i += 10)  //between research block and football ground
    {
        glPushMatrix();
        glTranslatef(-160, 0, i);
        trees();
        glPopMatrix();
    }

    for (int z = 25; z <= 45; z += 10) {
        for (int i = -140; i <= -110; i += 10) {
            glPushMatrix();
            glTranslatef(i, 0, z);
            trees();
            glPopMatrix();
        }
    }

    for (int i = -150; i <= -50; i += 10)  //in front of library (longer one)
    {
        glPushMatrix();
        glTranslatef(i, 0, 85);
        trees();
        glPopMatrix();
    }

    for (int i = -4; i <= 10; i += 10)  //side of temple
    {
        glPushMatrix();
        glTranslatef(i, 0, -100);
        trees();
        glPopMatrix();
    }

    for (int i = -130; i <= -100; i += 10)   //backside of temple
    {
        glPushMatrix();
        glTranslatef(15, 0, i);
        trees();
        glPopMatrix();
    }

    for (int i = -130; i <= -100; i += 10)   //backside of temple
    {
        glPushMatrix();
        glTranslatef(35, 0, i);
        trees();
        glPopMatrix();
    }

    for (int i = -130; i <= -100; i += 10)   //backside of temple
    {
        glPushMatrix();
        glTranslatef(25, 0, i);
        trees();
        glPopMatrix();
    }

    for (int i = -70; i <= 70; i += 10)
    {
        glPushMatrix();
        glTranslatef(120, 0, i);
        trees();
        glPopMatrix();
    }

    for (int i = -70; i <= 70; i += 10)
    {
        glPushMatrix();
        glTranslatef(110, 0, i);
        trees();
        glPopMatrix();
    }

    for (int i = -70; i <= 70; i += 10)
    {
        glPushMatrix();
        glTranslatef(130, 0, i);
        trees();
        glPopMatrix();
    }

    for (int i = -90; i <= 70; i += 10)
    {
        glPushMatrix();
        glTranslatef(140, 0, i);
        trees();
        glPopMatrix();
    }

    for (int i = -140; i <= -15; i += 10)
    {
        glPushMatrix();
        glTranslatef(i, 0, -160);
        trees();
        glPopMatrix();
    }

    for (int i = -140; i <= -15; i += 10)
    {
        glPushMatrix();
        glTranslatef(i, 0, -170);
        trees();
        glPopMatrix();
    }


    glPushMatrix();
    glTranslatef(-38, 0, 90);
    draw_arch("THAPAR");
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-70, 0, 80);
    canteen.disp_build("JAGGI", 'r');
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-10, 0, 10);
    admin.disp_build("ADMIN BLOCK");
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-10, 0, -30);
    cs.disp_build("CS BLOCK", 'l');
    glPopMatrix();

    glPushMatrix();
    glTranslatef(60, 0, -30);
    cs.disp_build("AUDITORIUM", 'l');
    glPopMatrix();

    glPushMatrix();
    glTranslatef(60, 0, 20);
    cs.disp_build("EC BLOCK", 'l');
    glPopMatrix();

    glPushMatrix();
    glTranslatef(60, 0, 60);
    library.disp_build("LIBRARY");
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-70, 0, -30);
    mba.disp_build("LT BLOCK", 'r');
    glPopMatrix();
    disp_mba();

    glPushMatrix();
    glTranslatef(-70, 0, 20);
    mech.disp_build("MECH BLOCK", 'r');
    glPopMatrix();
    mech_court();

    glPushMatrix();
    glTranslatef(-130, 0, 20);
    hospital.disp_build("HOSPITAL", 'r');
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-130, 0, -30);
    research.disp_build("RESEARCH", 'r');
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-130, 0, 80);
    ccos.disp_build("COS SHOPS", 'r');
    glPopMatrix();

    glPushMatrix();
    glTranslatef(-6, 0, -150);
    hostelA.disp_build("HOSTEL A", 'l');
    glPopMatrix();

    glPushMatrix();
    glTranslatef(40, 0, -100);
    admin.disp_build("HOSTEL B");
    glPopMatrix();

    glPushMatrix();
    glTranslatef(100, 0, -100);
    admin.disp_build("HOSTEL C");
    glPopMatrix();

    //temple
    glPushMatrix();
    glTranslatef(-6, 3, -120);
    temp.disp_temple();
    glPopMatrix();

    glPushMatrix();  //basketball court
    glTranslatef(-65, 0, 45);
    crt1.disp_court();
    glPopMatrix();

    badminton_ground_disp();
    badminton_ground_disp2();

    //football ground
    glPushMatrix();
    glTranslatef(-130, 0.0, -100);
    fball.ground_disp();
    glPopMatrix();

    // Draw Parking Area
    glColor3f(0.96f, 0.76f, 0.26f); // Sand color
    glBegin(GL_QUADS);
    glVertex3f(-10, 0.1f, 65);
    glVertex3f(30, 0.1f, 65);
    glVertex3f(30, 0.1f, 35);
    glVertex3f(-10, 0.1f, 35);
    glEnd();

    // Parameters for car grid
    int numColumns = 6;        // Cars per row
    int numRows = 3;           // Number of rows
    float xSpacing = 6.0f;     // Horizontal spacing
    float zSpacing = 5.0f;     // Depth spacing
    float startX = -8.0f;      // Starting x-position
    float startZ = 60.0f;      // Starting z-position (front row)

    // Draw cars in grid
    for (int row = 0; row < numRows; ++row) {
        for (int col = 0; col < numColumns; ++col) {
            float px = startX + col * xSpacing;
            float pz = startZ - row * zSpacing;
            draw_vehicle(
                px, 0.3f, pz,          // Position
                1.0f,                  // Scale
                carColors[0].r, carColors[0].g, carColors[0].b  // Color (use array if needed)
            );
        }
    }
    drawAllPeople();

    // show walking people
     // show parked cars

    glutSwapBuffers();
    glFlush();
}

// Special key functions
void pressKey(int key, int xx, int yy)
{
    switch (key) {
    case GLUT_KEY_UP:    // Move forward (arrow key up)
        deltaMove = 1.0f;
        glutIdleFunc(update);
        break;
    case GLUT_KEY_DOWN:  // Move backward (arrow key down)
        deltaMove = -1.0f;
        glutIdleFunc(update);
        break;

    }
}

void releaseKey(int key, int x, int y)
{
    switch (key) {
    case GLUT_KEY_UP:  // Stop moving forward
    case GLUT_KEY_DOWN:  // Stop moving backward
        deltaMove = 0.0f;
        glutIdleFunc(NULL);
        break;

    }
}

// Normal key functions
void pressNormalKey(unsigned char key, int xx, int yy)
{
    switch (key) {
    case 'u':   // Move up
        vertMove = 1.0f;
        glutIdleFunc(update);
        break;
    case 'j':   // Move down
        vertMove = -1.0f;
        glutIdleFunc(update);
        break;
    }
}

void releaseNormalKey(unsigned char key, int x, int y)
{
    switch (key) {
    case 'u':   // Stop moving up
    case 'j':   // Stop moving down
        vertMove = 0.0f;
        glutIdleFunc(NULL);
        break;
    }
}

// Mouse control functions 
void mouseMove(int x, int y)
{
    if (isDragging) {
        deltaAngle = (x - xDragStart) * -0.005;
        lx = sin(angle + deltaAngle);
        lz = -cos(angle + deltaAngle);
        glutIdleFunc(update);
    }
}

void mouseButton(int button, int state, int x, int y)
{
    if (button == GLUT_LEFT_BUTTON) {
        if (state == GLUT_DOWN) {
            isDragging = 1;
            xDragStart = x;
        }
        else {
            angle += deltaAngle;
            isDragging = 0;
            glutIdleFunc(NULL);
        }
    }
}

int main()
{
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInitWindowSize(500, 500);
    glutInitWindowPosition(40, 40);
    glutCreateWindow("CAMPUS");
    glutReshapeFunc(changeSize); 
    glutDisplayFunc(display);
    glutIgnoreKeyRepeat(1);
    glutSpecialFunc(pressKey);  
    glutSpecialUpFunc(releaseKey);  
    glutKeyboardFunc(pressNormalKey);   
    glutKeyboardUpFunc(releaseNormalKey); 
    glutMouseFunc(mouseButton); 
    glutMotionFunc(mouseMove); 
    glutMainLoop();
}
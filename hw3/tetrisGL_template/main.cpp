//
// Author: Ahmet Oguz Akyuz
//
// This is a sample code that draws a single block piece at the center
// of the window. It does many boilerplate work for you -- but no
// guarantees are given about the optimality and bug-freeness of the
// code. You can use it to get started or you can simply ignore its
// presence. You can modify it in any way that you like.
//
//

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#define _USE_MATH_DEFINES
#include <math.h>
#include <GL/glew.h>
// #include <OpenGL/gl3.h>   // The GL Header File
#include <GLFW/glfw3.h> // The GLFW header
#include <glm/glm.hpp>  // GL Math library header
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <ft2build.h>
#include FT_FREETYPE_H

#define BUFFER_OFFSET(i) ((char *)NULL + (i))

using namespace std;

GLuint gProgram[3];
int gWidth = 600, gHeight = 1000;
GLuint gVertexAttribBuffer, gTextVBO, gIndexBuffer;
GLuint gTex2D;
int gVertexDataSizeInBytes, gNormalDataSizeInBytes;
int gTriangleIndexDataSizeInBytes, gLineIndexDataSizeInBytes;

GLint modelingMatrixLoc[2];
GLint viewingMatrixLoc[2];
GLint projectionMatrixLoc[2];
GLint eyePosLoc[2];
GLint lightPosLoc[2];
GLint kdLoc[2];

glm::mat4 projectionMatrix;
glm::mat4 viewingMatrix;
glm::mat4 modelingMatrix = glm::translate(glm::mat4(1.f), glm::vec3(-0.5, -0.5, -0.5));
glm::vec3 eyePos = glm::vec3(0, 10, 26);

glm::vec3 lightPos = glm::vec3(0, 0, 7);

// glm::vec3 kdGround(0.334, 0.288, 0.635); // this is the ground color in the demo
glm::vec3 kdCubes(0.86, 0.11, 0.31);

glm::vec3 kdFloor(0.31, 0.11, 0.86);

int activeProgramIndex = 0;

// Holds all state information relevant to a character as loaded using FreeType
struct Character
{
    GLuint TextureID;   // ID handle of the glyph texture
    glm::ivec2 Size;    // Size of glyph
    glm::ivec2 Bearing; // Offset from baseline to left/top of glyph
    GLuint Advance;     // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;

struct UnitCube
{

    glm::vec3 pos; // relative locations of cube w.r.t center of the screen
    glm::vec3 scale;
    UnitCube(const glm::vec3 &position, const glm::vec3 &scaleValue)
        : pos(position), scale(scaleValue)
    {
    }
    bool operator<(const UnitCube &other) const
    {
        if (pos.y != other.pos.y)
            return pos.y < other.pos.y;
        if (pos.x != other.pos.x)
            return pos.x < other.pos.x;
        return pos.z < other.pos.z;
    }
};
struct Shape
{
    std::vector<glm::vec3> relativeCoordinates;
    Shape(const std::vector<glm::vec3> &coords) : relativeCoordinates(coords) {}
    std::vector<glm::vec3> rotateAroundYaxis90()
    {
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        std::vector<glm::vec3> tmpVector;
        for (auto &coord : relativeCoordinates)
        {
            glm::vec4 rotated = rotationMatrix * glm::vec4(coord, 1.0f);
            tmpVector.push_back(glm::vec3(std::round(rotated.x), std::round(rotated.y), std::round(rotated.z)));
        }

        alignCoordinates(tmpVector);
        return tmpVector;
    }

    std::vector<glm::vec3> rotateAroundXaxisNegative90()
    {
        glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));

        std::vector<glm::vec3> tmpVector;

        for (auto &coord : relativeCoordinates)
        {
            glm::vec4 rotated = rotationMatrix * glm::vec4(coord, 1.0f);
            tmpVector.push_back(glm::vec3(std::round(rotated.x), std::round(rotated.y), std::round(rotated.z)));
        }

        alignCoordinates(tmpVector);

        return tmpVector;
    }

private:
    void alignCoordinates(std::vector<glm::vec3> &coordinates)
    {
        float minX = std::numeric_limits<float>::max();
        float minY = std::numeric_limits<float>::max();
        float minZ = std::numeric_limits<float>::max();

        for (const auto &coord : coordinates)
        {
            minX = std::min(minX, coord.x);
            minY = std::min(minY, coord.y);
            minZ = std::min(minZ, coord.z);
        }

        for (auto &coord : coordinates)
        {
            coord.x -= minX;
            coord.y -= minY;
            coord.z -= minZ;
        }
    }
};

std::set<UnitCube> cubeSet; // all unit cubes except floor and current set of cubes we are placing
int num_blocks = 15;
float floor_pos = -8.0, floor_y_scale = 0.5;
float zeroYvalue = floor_pos + 2 + (floor_y_scale - 1);
const int gridSize = 9;
const GLfloat cubeSize = 1.0f; // Assuming each cube has a size of 1 unit
double offset = -cubeSize * (gridSize / 2);
glm::vec3 curpos(3, 12, 3), startpos(3, 15, 3);
double fallSpeed = 1.0f, minspeed = 0.0, maxSpeed = 2.0;
double fallInterval = 1.0 / fallSpeed; // Time in seconds between falls

// glm::vec3 rightVec(1.0f, 0.0f, 0.0f);
int curRightVecIndex = 0;
vector<glm::vec3> rightVecs = {glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)};
vector<string> vievStrings = {"Front", "Right", "Back", "Left"};
glm::vec3 zerovec = glm::vec3(0.0f, 0.0f, 0.0f);

double currentTime = glfwGetTime();
double lastFallTime = currentTime;
double lastRotation = currentTime;
double deltaTime;
double keyShowTime = 0.2f;
double lastKeyPressTime = -31.0;
double infoShowTime = currentTime + 10;
float currentRotation = 0.0f; // Current rotation angle in degrees
float targetRotation = 0.0f;  // Target rotation angle in degrees
float rotationTime = 0.15f;
int curScore = 0;
bool gameOver = false;
bool isInside = false;
bool isColliding = false;
bool differentShapesEnabled = true;
string pressedKey = "H";
std::vector<Shape> shapes = {
    // Cube (3x3x3)
    Shape({glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 0, 2),
           glm::vec3(0, 1, 0), glm::vec3(0, 1, 1), glm::vec3(0, 1, 2),
           glm::vec3(0, 2, 0), glm::vec3(0, 2, 1), glm::vec3(0, 2, 2),

           glm::vec3(1, 0, 0), glm::vec3(1, 0, 1), glm::vec3(1, 0, 2),
           glm::vec3(1, 1, 0), glm::vec3(1, 1, 1), glm::vec3(1, 1, 2),
           glm::vec3(1, 2, 0), glm::vec3(1, 2, 1), glm::vec3(1, 2, 2),

           glm::vec3(2, 0, 0), glm::vec3(2, 0, 1), glm::vec3(2, 0, 2),
           glm::vec3(2, 1, 0), glm::vec3(2, 1, 1), glm::vec3(2, 1, 2),
           glm::vec3(2, 2, 0), glm::vec3(2, 2, 1), glm::vec3(2, 2, 2)}),

    // Z Shape
    Shape({glm::vec3(0, 0, 0), glm::vec3(0, 0, 1),
           glm::vec3(1, 0, 1), glm::vec3(1, 0, 2)}),

    // L Shape
    Shape({glm::vec3(0, 0, 0), glm::vec3(1, 0, 0),
           glm::vec3(2, 0, 0), glm::vec3(2, 0, 1)}),

    // I Shape
    Shape({glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(2, 0, 0), glm::vec3(3, 0, 0)}),

    // L with 3 blocks
    Shape({glm::vec3(0, 0, 0), glm::vec3(1, 0, 0), glm::vec3(1, 0, 1)}),

    // Single Block
    Shape({glm::vec3(0, 0, 0)}),

    // 3x3 Square
    Shape({glm::vec3(0, 0, 0), glm::vec3(0, 0, 1), glm::vec3(0, 0, 2),
           glm::vec3(1, 0, 0), glm::vec3(1, 0, 1), glm::vec3(1, 0, 2),
           glm::vec3(2, 0, 0), glm::vec3(2, 0, 1), glm::vec3(2, 0, 2)})};

int cur_shape_index = 0;
int next_shape_index = 1;

std::vector<std::vector<int>> unitCubePositions = {
    {0, 0, 0}, {0, 0, 1}, {0, 0, 2}, {0, 1, 0}, {0, 1, 1}, {0, 1, 2}, {0, 2, 0}, {0, 2, 1}, {0, 2, 2},

    {1, 0, 0},
    {1, 0, 1},
    {1, 0, 2},
    {1, 1, 0},
    {1, 1, 1},
    {1, 1, 2},
    {1, 2, 0},
    {1, 2, 1},
    {1, 2, 2},

    {2, 0, 0},
    {2, 0, 1},
    {2, 0, 2},
    {2, 1, 0},
    {2, 1, 1},
    {2, 1, 2},
    {2, 2, 0},
    {2, 2, 1},
    {2, 2, 2}};

glm::mat4
getModelingMatrix(const glm::mat4 &baseMatrix, const UnitCube &unitCube, glm::vec3 cubeScale)
{
    glm::mat4 modelingMatrix = baseMatrix;

    modelingMatrix = glm::translate(modelingMatrix, (unitCube.pos * cubeScale) + glm::vec3(offset, zeroYvalue, offset));

    modelingMatrix = glm::scale(modelingMatrix, unitCube.scale);

    return modelingMatrix;
}

// For reading GLSL files
bool ReadDataFromFile(
    const string &fileName, ///< [in]  Name of the shader file
    string &data)           ///< [out] The contents of the file
{
    fstream myfile;

    // Open the input
    myfile.open(fileName.c_str(), std::ios::in);

    if (myfile.is_open())
    {
        string curLine;

        while (getline(myfile, curLine))
        {
            data += curLine;
            if (!myfile.eof())
            {
                data += "\n";
            }
        }

        myfile.close();
    }
    else
    {
        return false;
    }

    return true;
}

GLuint createVS(const char *shaderName)
{
    string shaderSource;

    string filename(shaderName);
    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar *shader = (const GLchar *)shaderSource.c_str();

    GLuint vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &shader, &length);
    glCompileShader(vs);

    char output[1024] = {0};
    glGetShaderInfoLog(vs, 1024, &length, output);
    printf("VS compile log: %s\n", output);

    return vs;
}

GLuint createFS(const char *shaderName)
{
    string shaderSource;

    string filename(shaderName);
    if (!ReadDataFromFile(filename, shaderSource))
    {
        cout << "Cannot find file name: " + filename << endl;
        exit(-1);
    }

    GLint length = shaderSource.length();
    const GLchar *shader = (const GLchar *)shaderSource.c_str();

    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &shader, &length);
    glCompileShader(fs);

    char output[1024] = {0};
    glGetShaderInfoLog(fs, 1024, &length, output);
    printf("FS compile log: %s\n", output);

    return fs;
}

void initFonts(int windowWidth, int windowHeight)
{
    // Set OpenGL options
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<GLfloat>(windowWidth), 0.0f, static_cast<GLfloat>(windowHeight));
    glUseProgram(gProgram[2]);
    glUniformMatrix4fv(glGetUniformLocation(gProgram[2], "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    // FreeType
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
    }

    // Load font as face
    FT_Face face;
    if (FT_New_Face(ft, "/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf", 0, &face))
    // if (FT_New_Face(ft, "/usr/share/fonts/truetype/gentium-basic/GenBkBasR.ttf", 0, &face)) // you can use different fonts
    {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
    }

    // Set size to load glyphs as
    FT_Set_Pixel_Sizes(face, 0, 48);

    // Disable byte-alignment restriction
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    // Load first 128 characters of ASCII set
    for (GLubyte c = 0; c < 128; c++)
    {
        // Load character glyph
        if (FT_Load_Char(face, c, FT_LOAD_RENDER))
        {
            std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
            continue;
        }
        // Generate texture
        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RED,
            face->glyph->bitmap.width,
            face->glyph->bitmap.rows,
            0,
            GL_RED,
            GL_UNSIGNED_BYTE,
            face->glyph->bitmap.buffer);
        // Set texture options
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        // Now store character for later use    int num_blocks = 15;

        Character character = {
            texture,
            glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
            glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
            (GLuint)face->glyph->advance.x};
        Characters.insert(std::pair<GLchar, Character>(c, character));
    }

    glBindTexture(GL_TEXTURE_2D, 0);
    // Destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    //
    // Configure VBO for texture quads
    //
    glGenBuffers(1, &gTextVBO);
    glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(GLfloat) * 6 * 4, NULL, GL_DYNAMIC_DRAW);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), 0);

    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void initShaders()
{
    // Create the programs

    gProgram[0] = glCreateProgram();
    gProgram[1] = glCreateProgram();
    gProgram[2] = glCreateProgram();

    // Create the shaders for both programs

    GLuint vs1 = createVS("vert.glsl"); // for cube shading
    GLuint fs1 = createFS("frag.glsl");

    GLuint vs2 = createVS("vert2.glsl"); // for border shading
    GLuint fs2 = createFS("frag2.glsl");

    GLuint vs3 = createVS("vert_text.glsl"); // for text shading
    GLuint fs3 = createFS("frag_text.glsl");

    // Attach the shaders to the programs

    glAttachShader(gProgram[0], vs1);
    glAttachShader(gProgram[0], fs1);

    glAttachShader(gProgram[1], vs2);
    glAttachShader(gProgram[1], fs2);

    glAttachShader(gProgram[2], vs3);
    glAttachShader(gProgram[2], fs3);

    // Link the programs

    for (int i = 0; i < 3; ++i)
    {
        glLinkProgram(gProgram[i]);
        GLint status;
        glGetProgramiv(gProgram[i], GL_LINK_STATUS, &status);

        if (status != GL_TRUE)
        {
            cout << "Program link failed: " << i << endl;
            exit(-1);
        }
    }

    // Get the locations of the uniform variables from both programs

    for (int i = 0; i < 2; ++i)
    {
        modelingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "modelingMatrix");
        viewingMatrixLoc[i] = glGetUniformLocation(gProgram[i], "viewingMatrix");
        projectionMatrixLoc[i] = glGetUniformLocation(gProgram[i], "projectionMatrix");
        eyePosLoc[i] = glGetUniformLocation(gProgram[i], "eyePos");
        lightPosLoc[i] = glGetUniformLocation(gProgram[i], "lightPos");
        kdLoc[i] = glGetUniformLocation(gProgram[i], "kd");

        glUseProgram(gProgram[i]);
        glUniformMatrix4fv(modelingMatrixLoc[i], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
        glUniform3fv(eyePosLoc[i], 1, glm::value_ptr(eyePos));
        glUniform3fv(lightPosLoc[i], 1, glm::value_ptr(lightPos));
        glUniform3fv(kdLoc[i], 1, glm::value_ptr(kdCubes));
    }
}

// VBO setup for drawing a cube and its borders
void initVBO()
{
    GLuint vao;
    glGenVertexArrays(1, &vao);
    assert(vao > 0);
    glBindVertexArray(vao);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    assert(glGetError() == GL_NONE);

    glGenBuffers(1, &gVertexAttribBuffer);
    glGenBuffers(1, &gIndexBuffer);

    assert(gVertexAttribBuffer > 0 && gIndexBuffer > 0);

    glBindBuffer(GL_ARRAY_BUFFER, gVertexAttribBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, gIndexBuffer);

    GLuint indices[] = {
        0, 1, 2, // front
        3, 0, 2, // front
        4, 7, 6, // back
        5, 4, 6, // back
        0, 3, 4, // left
        3, 7, 4, // left
        2, 1, 5, // right
        6, 2, 5, // right
        3, 2, 7, // top
        2, 6, 7, // top
        0, 4, 1, // bottom
        4, 5, 1  // bottom
    };

    GLuint indicesLines[] = {
        7, 3, 2, 6, // top
        4, 5, 1, 0, // bottom
        2, 1, 5, 6, // right
        5, 4, 7, 6, // back
        0, 1, 2, 3, // front
        0, 3, 7, 4, // left
    };

    GLfloat vertexPos[] = {
        0, 0, 1, // 0: bottom-left-front
        1, 0, 1, // 1: bottom-right-front
        1, 1, 1, // 2: top-right-front
        0, 1, 1, // 3: top-left-front
        0, 0, 0, // 0: bottom-left-back
        1, 0, 0, // 1: bottom-right-back
        1, 1, 0, // 2: top-right-back
        0, 1, 0, // 3: top-left-back
    };

    GLfloat vertexNor[] = {
        1.0, 1.0, 1.0,  // 0: unused
        0.0, -1.0, 0.0, // 1: bottom
        0.0, 0.0, 1.0,  // 2: front
        1.0, 1.0, 1.0,  // 3: unused
        -1.0, 0.0, 0.0, // 4: left
        1.0, 0.0, 0.0,  // 5: right
        0.0, 0.0, -1.0, // 6: back
        0.0, 1.0, 0.0,  // 7: top
    };

    gVertexDataSizeInBytes = sizeof(vertexPos);
    gNormalDataSizeInBytes = sizeof(vertexNor);
    gTriangleIndexDataSizeInBytes = sizeof(indices);
    gLineIndexDataSizeInBytes = sizeof(indicesLines);
    int allIndexSize = gTriangleIndexDataSizeInBytes + gLineIndexDataSizeInBytes;

    glBufferData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes + gNormalDataSizeInBytes, 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, gVertexDataSizeInBytes, vertexPos);
    glBufferSubData(GL_ARRAY_BUFFER, gVertexDataSizeInBytes, gNormalDataSizeInBytes, vertexNor);

    glBufferData(GL_ELEMENT_ARRAY_BUFFER, allIndexSize, 0, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, gTriangleIndexDataSizeInBytes, indices);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, gTriangleIndexDataSizeInBytes, gLineIndexDataSizeInBytes, indicesLines);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, BUFFER_OFFSET(gVertexDataSizeInBytes));
}

void init()
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    // polygon offset is used to prevent z-fighting between the cube and its borders
    glPolygonOffset(0.5, 0.5);
    glEnable(GL_POLYGON_OFFSET_FILL);

    initShaders();
    initVBO();
    initFonts(gWidth, gHeight);
}

void drawCube()
{
    glUseProgram(gProgram[0]);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);
}

void drawCubeEdges()
{
    glLineWidth(3);

    glUseProgram(gProgram[1]);

    for (int i = 0; i < 6; ++i)
    {
        glDrawElements(GL_LINE_LOOP, 4, GL_UNSIGNED_INT, BUFFER_OFFSET(gTriangleIndexDataSizeInBytes + i * 4 * sizeof(GLuint)));
    }
}
void updateModelingMatrixesInShaders(glm::mat4 modelingMatrix)
{
    glUseProgram(gProgram[0]);
    glUniformMatrix4fv(modelingMatrixLoc[0], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
    glUseProgram(gProgram[1]);
    glUniformMatrix4fv(modelingMatrixLoc[1], 1, GL_FALSE, glm::value_ptr(modelingMatrix));
}
glm::mat4 getCurModelingMatrix()
{
    GLfloat matrixValues[16];
    glUseProgram(gProgram[0]);
    glGetUniformfv(gProgram[0], modelingMatrixLoc[0], matrixValues);
    return glm::make_mat4(matrixValues);
}

// changes current modeling matrix ,save current first
void drawUnitCube(const UnitCube &cube, const glm::mat4 &curModelingMatrix)
{

    // Base modeling matrix
    glm::mat4 baseMatrix = curModelingMatrix;

    glm::mat4 modelingMatrix = getModelingMatrix(baseMatrix, cube, cube.scale);

    assert(glGetError() == GL_NO_ERROR);
    // Send the modeling matrix to the shader
    updateModelingMatrixesInShaders(modelingMatrix);

    // Draw the cube and its edges
    drawCube();
    drawCubeEdges();
}
void drawCurrentShape(glm ::vec3 pos)
{
    modelingMatrix = getCurModelingMatrix();

    for (auto &coordinates : shapes[cur_shape_index].relativeCoordinates)
    {
        UnitCube cube(glm::vec3(coordinates[0] + pos.x, coordinates[1] + pos.y, coordinates[2] + pos.z),
                      glm::vec3(1, 1, 1));
        drawUnitCube(cube, modelingMatrix);
    }
    updateModelingMatrixesInShaders(modelingMatrix);
}
void drawNextShape(Shape &shape, glm::vec3 offset, float scale)
{

    GLfloat matrixValues[16];
    glUseProgram(gProgram[0]);
    glGetUniformfv(gProgram[0], modelingMatrixLoc[0], matrixValues);
    glm::mat4 tmpModelingMat = glm::make_mat4(matrixValues);

    // Base modeling matrix
    glm::mat4 baseMatrix = modelingMatrix;

    glUseProgram(gProgram[0]);
    glUniform3fv(kdLoc[0], 1, glm::value_ptr(kdFloor));
    // Iterate through the grid and draw each cube

    for (auto &relativePosition : shape.relativeCoordinates)
    {

        UnitCube cube(offset + glm::vec3(relativePosition[0], relativePosition[1], relativePosition[2]),
                      glm::vec3(scale, scale, scale));
        glm::mat4 modelingMatrix = getModelingMatrix(baseMatrix, cube, glm::vec3(scale, scale, scale));

        assert(glGetError() == GL_NO_ERROR);
        // Send the modeling matrix to the shader
        updateModelingMatrixesInShaders(modelingMatrix);

        // Draw the cube and its edges
        drawCube();
        drawCubeEdges();
    }

    glUseProgram(gProgram[0]);
    glUniform3fv(kdLoc[0], 1, glm::value_ptr(kdCubes));
    updateModelingMatrixesInShaders(tmpModelingMat);
}
int chooseNextShapeIndex()
{
    if (!differentShapesEnabled)
        return 0;
    return (next_shape_index + 1) % shapes.size();
    return random() % shapes.size();
}
void draw3x3x3CubeGrid(glm ::vec3 pos)
{
    // 3x3x3 grid size
    const int gridSize = 3;
    modelingMatrix = getCurModelingMatrix();
    // Base modeling matrix

    // Iterate through the grid and draw each cube
    for (int x = 0; x < gridSize; x++)
    {
        for (int y = 0; y < gridSize; y++)
        {
            for (int z = 0; z < gridSize; z++)
            {
                UnitCube cube(glm::vec3(x + pos.x, y + pos.y, z + pos.z),
                              glm::vec3(1, 1, 1));
                drawUnitCube(cube, modelingMatrix);
            }
        }
    }

    updateModelingMatrixesInShaders(modelingMatrix);
}
void drawFloor(float floor_y_scale)
{

    GLfloat matrixValues[16];
    glUseProgram(gProgram[0]);
    glGetUniformfv(gProgram[0], modelingMatrixLoc[0], matrixValues);
    glm::mat4 tmpModelingMat = glm::make_mat4(matrixValues);

    // Base modeling matrix
    glm::mat4 baseMatrix = modelingMatrix;

    glUseProgram(gProgram[0]);
    glUniform3fv(kdLoc[0], 1, glm::value_ptr(kdFloor));
    // Iterate through the grid and draw each cube
    for (int x = 0; x < gridSize; ++x)
    {

        for (int z = 0; z < gridSize; ++z)
        {
            // Calculate the translation matrix for the current cube

            UnitCube cube(glm::vec3(x, -0.5, z),
                          glm::vec3(1.0, floor_y_scale, 1.0));
            glm::mat4 modelingMatrix = getModelingMatrix(baseMatrix, cube, glm::vec3(1.0f, 1.0f, 1.0f));

            assert(glGetError() == GL_NO_ERROR);
            // Send the modeling matrix to the shader
            updateModelingMatrixesInShaders(modelingMatrix);

            // Draw the cube and its edges
            drawCube();
            drawCubeEdges();
        }
    }
    glUseProgram(gProgram[0]);
    glUniform3fv(kdLoc[0], 1, glm::value_ptr(kdCubes));
    updateModelingMatrixesInShaders(tmpModelingMat);
}
float calculateTextWidth(const std::string &text, float scale)
{
    float width = 0.0f;
    for (const char &c : text)
    {
        Character ch = Characters[c];
        width += (ch.Advance >> 6) * scale;
    }
    return width;
}

void renderText(const std::string &text, GLfloat x, GLfloat y, GLfloat scale, glm::vec3 color)
{
    // Activate corresponding render state
    glUseProgram(gProgram[2]);
    glUniform3f(glGetUniformLocation(gProgram[2], "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);

    // Iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++)
    {
        Character ch = Characters[*c];

        GLfloat xpos = x + ch.Bearing.x * scale;
        GLfloat ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        GLfloat w = ch.Size.x * scale;
        GLfloat h = ch.Size.y * scale;

        // Update VBO for each character
        GLfloat vertices[6][4] = {
            {xpos, ypos + h, 0.0, 0.0},
            {xpos, ypos, 0.0, 1.0},
            {xpos + w, ypos, 1.0, 1.0},

            {xpos, ypos + h, 0.0, 0.0},
            {xpos + w, ypos, 1.0, 1.0},
            {xpos + w, ypos + h, 1.0, 0.0}};

        // Render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);

        // Update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, gTextVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // Be sure to use glBufferSubData and not glBufferData

        // glBindBuffer(GL_ARRAY_BUFFER, 0);

        // Render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // Now advance cursors for next glyph (note that advance is number of 1/64 pixels)

        x += (ch.Advance >> 6) * scale; // Bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}
void insertNewCubes(glm::vec3 newpos)
{

    for (auto &relaiveCoordinates : shapes[cur_shape_index].relativeCoordinates)
    {
        UnitCube cube(glm::vec3(relaiveCoordinates[0] + newpos.x, relaiveCoordinates[1] + newpos.y, relaiveCoordinates[2] + newpos.z),
                      glm::vec3(1, 1, 1));
        cubeSet.insert(cube);
    }
}
void drawCurrentBlocks()
{
    auto tmp = getCurModelingMatrix();
    for (auto &cube : cubeSet)
    {
        drawUnitCube(cube, tmp);
    }
    updateModelingMatrixesInShaders(tmp);
}
bool removeFullRows()
{
    bool res = false;
    vector<int> blockCountPerYcoordinate(num_blocks, 0);
    vector<int> ehe;
    for (const UnitCube &cube : cubeSet)
    {
        blockCountPerYcoordinate[cube.pos.y]++;
    }
    for (int i = num_blocks; i >= 0; i--)
    {
        if (blockCountPerYcoordinate[i] == 81)
        {
            ehe.push_back(i);
            curScore += 81;
            res = true;
        }
    }
    for (int y : ehe)
    {
        auto it = cubeSet.begin();
        while (it != cubeSet.end())
        {
            if (it->pos.y == y)
            {
                it = cubeSet.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }
    for (int y : ehe)
    {
        auto it = cubeSet.begin();

        while (it != cubeSet.end())
        {
            if (it->pos.y > y)
            {
                UnitCube modifiedCube = *it;
                modifiedCube.pos.y--;
                it = cubeSet.erase(it);
                cubeSet.insert(modifiedCube);
            }
            else
                it++;
        }
    }
    return res;
}
bool handleCollision(glm::vec3 offset, bool instantly)
{
    isInside = false;
    isColliding = false;
    glm::vec3 tmp = curpos + glm::vec3(0.0f, -1.0f, 0.0f) + offset;
    UnitCube cube(curpos, glm::vec3(1.0f, 1.0f, 1.0f));
    UnitCube cube2(tmp, glm::vec3(1.0f, 1.0f, 1.0f));
    if (!gameOver)
    {

        for (auto &relativeCubeCoordinates : shapes[cur_shape_index].relativeCoordinates)
        {
            cube.pos = curpos + glm::vec3(relativeCubeCoordinates[0], relativeCubeCoordinates[1], relativeCubeCoordinates[2]);
            cube2.pos = tmp + glm::vec3(relativeCubeCoordinates[0], relativeCubeCoordinates[1], relativeCubeCoordinates[2]);

            if (cubeSet.find(cube) != cubeSet.end())
            {
                isInside = true;
                break;
            }

            if (cubeSet.find(cube2) != cubeSet.end())
            {
                isColliding = true;
            }
        }
        if (curpos.y + offset.y == 0.0)
        {
            isColliding = true;
        }
        if (isInside)
        {
            insertNewCubes(curpos);
            gameOver = true;
        }
    }
    if ((!gameOver && (instantly || (fallSpeed != 0.0 && currentTime > lastFallTime + fallInterval))))
    {

        if (isColliding)
        {

            insertNewCubes(curpos + offset);

            removeFullRows();
            curpos = startpos;

            cur_shape_index = next_shape_index;
            next_shape_index = chooseNextShapeIndex();
        }
        else
        {
            if (!instantly)
                curpos = curpos + glm::vec3(0.0f, -1.0f, 0.0f);
        }

        lastFallTime = currentTime;
    }
    return isColliding;
}

void display()
{
    glClearColor(0, 0, 0, 1);
    glClearDepth(1.0f);
    glClearStencil(0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    drawCurrentBlocks();
    if (!gameOver)
    {
        drawNextShape(shapes[next_shape_index], startpos * 2.f + glm ::vec3(1.5, 3.0, 1.5), 0.5);
        //drawNextShape(shapes[next_shape_index], startpos , 1);
        drawCurrentShape(curpos);
    }

    drawFloor(floor_y_scale);
    handleCollision(zerovec, false);
    float hor_scale = 0.005;
    string score_string = std::to_string(curScore);
    float score_scale = 0.65;
    int text_width = calculateTextWidth(score_string, score_scale);
    renderText("Score:" + score_string, gWidth * hor_scale + 500 - text_width, gHeight * hor_scale + 960, score_scale, glm::vec3(0, 1, 0));
    renderText(vievStrings[curRightVecIndex], gWidth * hor_scale, gHeight * hor_scale + 960, score_scale, glm::vec3(0, 1, 0));
    if (currentTime < infoShowTime)
    {
        renderText("Press c and v keys for rotation around x and y axes,press z", 80 + gWidth * hor_scale, gHeight * hor_scale + 964, 0.3, glm::vec3(0, 1, 0));
        renderText("Press z for enabling/disabling different shapes", 80 + gWidth * hor_scale, gHeight * hor_scale + 964-16, 0.3, glm::vec3(0, 1, 0));
        renderText("Press x for toggling the next shape", 80 + gWidth * hor_scale, gHeight * hor_scale + 964-32, 0.3, glm::vec3(0, 1, 0));
        renderText("Press space for placing the shape instantly", 80 + gWidth * hor_scale, gHeight * hor_scale + 964-48, 0.3, glm::vec3(0, 1, 0));
    }
    if (currentTime < lastKeyPressTime + keyShowTime)
    {
        renderText(pressedKey, gWidth * hor_scale, gHeight * hor_scale + 920, 0.65, glm::vec3(0, 1, 1));
    }
    if (gameOver)
    {
        renderText("GAME OVER!", gWidth * hor_scale + 120, gHeight * hor_scale + 450, 1.31, glm::vec3(0.52, 0.31, 0.69));
    }

    assert(glGetError() == GL_NO_ERROR);
}

void reshape(GLFWwindow *window, int w, int h)
{
    w = w < 1 ? 1 : w;
    h = h < 1 ? 1 : h;

    gWidth = w;
    gHeight = h;

    glViewport(0, 0, w, h);

    // Use perspective projection

    float fovyRad = (float)(45.0 / 180.0) * M_PI;
    projectionMatrix = glm::perspective(fovyRad, gWidth / (float)gHeight, 1.0f, 100.0f);

    // always look toward (0, 0, 0)
    viewingMatrix = glm::lookAt(eyePos, glm::vec3(0, 1, 0), glm::vec3(0, 1, 0));

    for (int i = 0; i < 2; ++i)
    {
        glUseProgram(gProgram[i]);
        glUniformMatrix4fv(projectionMatrixLoc[i], 1, GL_FALSE, glm::value_ptr(projectionMatrix));
        glUniformMatrix4fv(viewingMatrixLoc[i], 1, GL_FALSE, glm::value_ptr(viewingMatrix));
    }
}

inline bool
checkInside(glm::vec3 &vec)
{
    return vec.x >= 0.0f && vec.x < 9.0f && vec.z >= 0.0f && vec.z < 9.0f;
}
bool checkShapeInside(Shape &s, glm::vec3 &pos)
{
    UnitCube cube(pos, glm::vec3(1, 1, 1));
    bool flag = true;

    for (auto &relativeCoordinates : s.relativeCoordinates)
    {
        cube.pos = pos + glm::vec3(relativeCoordinates[0], relativeCoordinates[1], relativeCoordinates[2]);
        if (!checkInside(cube.pos) || cubeSet.find(cube) != cubeSet.end())
        {
            flag = false;
            break;
        }
    }
    return flag;
}
void moveBlock(glm::vec3 delta)

{
    glm::vec3 tmp = curpos + delta;
    if (checkShapeInside(shapes[cur_shape_index], tmp))
    {
        curpos = tmp;
    }
}

void changeSpeed(double delta)
{
    double newspeed = max(0.0, fallSpeed + delta);
    if (fallSpeed == 0.0 && newspeed > 0.0)
    {
        lastFallTime = currentTime;
    }
    fallSpeed = newspeed;
    if (fallSpeed != 0.0)
    {
        fallInterval = 1.0 / fallSpeed;
    }
}

void changeRotationTarget(double delta)
{
    targetRotation += delta;
    if (targetRotation < 0)
    {
        targetRotation += 360.0;
        currentRotation += 360.0;
    }
    if (targetRotation > 360.0)
    {
        targetRotation -= 360.0;
        currentRotation -= 360.0;
    }

    curRightVecIndex = delta > 0 ? (curRightVecIndex + 1) % 4 : (curRightVecIndex - 1 + 4) % 4;
}
void rotateCurrentShapeX()
{
    auto rotated = shapes[cur_shape_index].rotateAroundXaxisNegative90();
    Shape nShape = Shape(rotated);
    if (checkShapeInside(nShape, curpos))
    {
        shapes[cur_shape_index] = nShape;
    }
}
void rotateCurrentShapeY()
{
    auto rotated = shapes[cur_shape_index].rotateAroundYaxis90();

    Shape nShape = Shape(rotated);
    if (checkShapeInside(nShape, curpos))
    {
        shapes[cur_shape_index] = nShape;
    }
}
void handleSpaceKey()
{
    int azd_ehe = curpos.y;
    for (int i = 0; i <= azd_ehe; i++)
    {
        if (handleCollision(glm::vec3(0.0f, (GLfloat)(-i), 0.0f), true))
        {
            return;
        }
    }
}
void keyboard(GLFWwindow *window, int key, int scancode, int action, int mods)
{
    if ((key == GLFW_KEY_Q || key == GLFW_KEY_ESCAPE) && action == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        auto tmpPresstime = lastKeyPressTime;
        lastKeyPressTime = currentTime;
        switch (key)
        {
        case GLFW_KEY_A:
            pressedKey = "A";
            if (!gameOver)
                moveBlock(rightVecs[curRightVecIndex] * -1.0f);
            break;
        case GLFW_KEY_D:
            pressedKey = "D";
            if (!gameOver)

                moveBlock(rightVecs[curRightVecIndex]);
            break;
        case GLFW_KEY_S:
            pressedKey = "S";
            changeSpeed(0.2);
            break;
        case GLFW_KEY_W:
            pressedKey = "W";
            changeSpeed(-0.2);
            break;
        case GLFW_KEY_H:
            pressedKey = "H";
            changeRotationTarget(-90.0);
            break;
        case GLFW_KEY_K:
            pressedKey = "K";
            changeRotationTarget(90.0);
            break;
        case GLFW_KEY_V:
            pressedKey = "V";
            if (!gameOver)
                rotateCurrentShapeY();
            break;
        case GLFW_KEY_C:
            pressedKey = "C";
            if (!gameOver)
                rotateCurrentShapeX();
            break;
        case GLFW_KEY_X:
            pressedKey = "X";
            next_shape_index = chooseNextShapeIndex();
            break;
        case GLFW_KEY_Z:
            pressedKey = "Z";
            differentShapesEnabled = !differentShapesEnabled;
            next_shape_index = chooseNextShapeIndex();
            break;
        case GLFW_KEY_SPACE:
            pressedKey = "Space";
            handleSpaceKey();
            break;
        default:
            lastKeyPressTime = tmpPresstime;
            break;
        }
    }
}
void rotateCam(GLFWwindow *window)
{

    if (fabs(targetRotation - currentRotation) < 1)
    {
        return;
    }
    double degrees = (deltaTime / rotationTime) * (targetRotation - currentRotation);
    double sign = glm::sign(degrees);
    degrees = sign * round(abs(degrees));
    if (degrees == 0)
        degrees = sign;
    currentRotation += degrees;

    float angleInRadians = glm::radians(degrees);
    glm::mat4 rotationMatrix = glm::rotate(glm::mat4(1.0f), angleInRadians, glm::vec3(0.0f, 1.0f, 0.0f));

    eyePos = glm::vec3(rotationMatrix * glm::vec4(eyePos, 1.0f));

    lightPos = glm::vec3(rotationMatrix * glm::vec4(lightPos, 0.0f));
    glUseProgram(gProgram[0]);
    glUniform3fv(lightPosLoc[0], 1, glm::value_ptr(lightPos));
    glUseProgram(gProgram[1]);

    glUniform3fv(lightPosLoc[1], 1, glm::value_ptr(lightPos));

    reshape(window, gWidth, gHeight);
}
void mainLoop(GLFWwindow *window)
{
    while (!glfwWindowShouldClose(window))
    {
        lastRotation = currentTime;
        currentTime = glfwGetTime();
        deltaTime = currentTime - lastRotation;

        rotateCam(window);
        display();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
}

int main(int argc, char **argv) // Create Main Function For Bringing It All Together
{

    GLFWwindow *window;
    if (!glfwInit())
    {
        exit(-1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(gWidth, gHeight, "tetrisGL", NULL, NULL);

    if (!window)
    {
        glfwTerminate();
        exit(-1);
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    // Initialize GLEW to setup the OpenGL Function pointers
    if (GLEW_OK != glewInit())
    {
        std::cout << "Failed to initialize GLEW" << std::endl;
        return EXIT_FAILURE;
    }

    char rendererInfo[512] = {0};
    strcpy(rendererInfo, (const char *)glGetString(GL_RENDERER));
    strcat(rendererInfo, " - ");
    strcat(rendererInfo, (const char *)glGetString(GL_VERSION));
    glfwSetWindowTitle(window, rendererInfo);

    init();

    glfwSetKeyCallback(window, keyboard);
    glfwSetWindowSizeCallback(window, reshape);

    reshape(window, gWidth, gHeight); // need to call this once ourselves
    mainLoop(window);                 // this does not return unless the window is closed

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}

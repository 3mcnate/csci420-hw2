/*
  CSCI 420 Computer Graphics, USC
  Assignment 2: Roller Coaster
  C/C++ starter code

  Student username: nboxer
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "openGLHeader.h"
#include "glutHeader.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "pipelineProgram.h"
#include "vbo.h"
#include "vao.h"
#include "hw1-helper.h"
#include "hw2-helper.h"

#include <iostream>
#include <cstring>
#include <memory>
#include <vector>
#include <chrono>

#if defined(WIN32) || defined(_WIN32)
#ifdef _DEBUG
#pragma comment(lib, "glew32d.lib")
#else
#pragma comment(lib, "glew32.lib")
#endif
#endif

#if defined(WIN32) || defined(_WIN32)
char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
char shaderBasePath[1024] = ".";
#endif

using namespace std;

int mousePos[2]; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0;   // 1 if pressed, 0 if not
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0;  // 1 if pressed, 0 if not

bool zPressed = false;

typedef enum
{
  ROTATE,
  TRANSLATE,
  SCALE
} CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

typedef enum
{
  POINTS,
  LINES,
  TRIANGLES,
  SMOOTH_TRIANGLES
} RENDER_MODE;
RENDER_MODE renderMode = POINTS;

int shaderMode = 0;
float scale = 1;
float exponent = 1;

// Transformations of the terrain.
float terrainRotate[3] = {0.0f, 0.0f, 0.0f};
// terrainRotate[0] gives the rotation around x-axis (in degrees)
// terrainRotate[1] gives the rotation around y-axis (in degrees)
// terrainRotate[2] gives the rotation around z-axis (in degrees)
float terrainTranslate[3] = {0.0f, 0.0f, 0.0f};
float terrainScale[3] = {1.0f, 1.0f, 1.0f};

// Width and height of the OpenGL window, in pixels.
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI 420 Homework 2";

// Number of vertices in the single triangle (starter code).
int numPointVertices;
int numLineVertices;
int numTriangleVertices;

// CSCI 420 helper classes.
OpenGLMatrix matrix;
PipelineProgram pipelineProgram;

// points
VBO pointsVboVertices;
VBO pointsVboColors;
VAO pointsVao;

// lines
VBO linesVboVertices;
VBO linesVboColors;
VAO linesVao;

// triangles
VBO trianglesVboVertices;
VBO trianglesVboColors;
VAO trianglesVao;

// smooth triangles
VBO centerSmoothTriangesVBO;
VBO leftSmoothTriangesVBO;
VBO rightSmoothTriangesVBO;
VBO upSmoothTriangesVBO;
VBO downSmoothTriangesVBO;
VBO colorSmoothTriangesVBO;
VAO smoothTrianglesVAO;

bool animate = false;
auto lastScreenshot = std::chrono::system_clock::now();
int screenshotCounter = 0;

// Write a screenshot to the specified filename.
void saveScreenshot(const char *filename)
{
  std::unique_ptr<unsigned char[]> screenshotData = std::make_unique<unsigned char[]>(windowWidth * windowHeight * 3);
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData.get());

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData.get());

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else
    cout << "Failed to save file " << filename << '.' << endl;
}

void idleFunc()
{
  // Do some stuff...
  // For example, here, you can save the screenshots to disk (to make the animation).
  auto now = std::chrono::system_clock::now();
  if (animate && std::chrono::duration_cast<std::chrono::milliseconds>(now - lastScreenshot).count() >= 100 && screenshotCounter < 300)
  {
    char buf[32];
    snprintf(buf, 32, "animation1/%03d.jpg", screenshotCounter++);
    saveScreenshot(buf);
    lastScreenshot = std::chrono::system_clock::now();
  }

  // Notify GLUT that it should call displayFunc.
  glutPostRedisplay();
}

void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  // When the window has been resized, we need to re-set our projection matrix.
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  // You need to be careful about setting the zNear and zFar.
  // Anything closer than zNear, or further than zFar, will be culled.
  const float zNear = 0.1f;
  const float zFar = 10000.0f;
  const float humanFieldOfView = 60.0f;
  matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

void mouseMotionDragFunc(int x, int y)
{
  // Mouse has moved, and one of the mouse buttons is pressed (dragging).

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = {x - mousePos[0], y - mousePos[1]};

  switch (controlState)
  {
  // translate the terrain
  case TRANSLATE:
    if (leftMouseButton)
    {
      // control x,y translation via the left mouse button
      terrainTranslate[0] += mousePosDelta[0] * 0.01f;
      terrainTranslate[1] -= mousePosDelta[1] * 0.01f;
    }
    if (middleMouseButton)
    {
      // control z translation via the middle mouse button
      terrainTranslate[2] += mousePosDelta[1] * 0.01f;
    }
    break;

  // rotate the terrain
  case ROTATE:
    if (leftMouseButton)
    {
      // control x,y rotation via the left mouse button
      terrainRotate[0] += mousePosDelta[1];
      terrainRotate[1] += mousePosDelta[0];
    }
    if (middleMouseButton)
    {
      // control z rotation via the middle mouse button
      terrainRotate[2] += mousePosDelta[1];
    }
    break;

  // scale the terrain
  case SCALE:
    if (leftMouseButton)
    {
      // control x,y scaling via the left mouse button
      terrainScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
      terrainScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
    }
    if (middleMouseButton)
    {
      // control z scaling via the middle mouse button
      terrainScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
    }
    break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // Mouse has moved.
  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // A mouse button has has been pressed or depressed.

  // Keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables.
  switch (button)
  {
  case GLUT_LEFT_BUTTON:
    leftMouseButton = (state == GLUT_DOWN);
    break;

  case GLUT_MIDDLE_BUTTON:
    middleMouseButton = (state == GLUT_DOWN);
    break;

  case GLUT_RIGHT_BUTTON:
    rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  if (glutGetModifiers() & GLUT_ACTIVE_CTRL)
  {
    cout << "CTRL pressed" << endl;
  }

  // Keep track of whether CTRL and SHIFT keys are pressed.
  switch (glutGetModifiers())
  {
  case GLUT_ACTIVE_CTRL:
    controlState = TRANSLATE;
    break;

  case GLUT_ACTIVE_SHIFT:
    controlState = SCALE;
    break;

  // If CTRL and SHIFT are not pressed, we are in rotate mode.
  default:
    controlState = ROTATE;
    break;
  }

  if (zPressed)
    controlState = TRANSLATE;

  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
  case 27:   // ESC key
    exit(0); // exit the program
    break;

  case ' ':
    cout << "You pressed the spacebar." << endl;
    break;

  case 'x':
    // Take a screenshot.
    saveScreenshot("screenshot.jpg");
    break;

  case 'z':
    zPressed = true;
    break;

  case '1':
    renderMode = POINTS;
    shaderMode = 0;
    break;

  case '2':
    renderMode = LINES;
    shaderMode = 0;
    break;

  case '3':
    renderMode = TRIANGLES;
    shaderMode = 0;
    break;

  case '4':
    renderMode = SMOOTH_TRIANGLES;
    shaderMode = 1;
    break;

  case '+':
    scale *= 2.0;
    break;

  case '-':
    scale /= 2.0;
    break;

  case '9':
    exponent *= 2.0;
    break;

  case '0':
    exponent /= 2.0;
    break;
  }

  pipelineProgram.SetUniformVariablei("mode", shaderMode);
  pipelineProgram.SetUniformVariablef("scale", scale);
  pipelineProgram.SetUniformVariablef("exponent", exponent);
}

void keyboardUpFunc(unsigned char key, int x, int y)
{
  if (key == 'z')
    zPressed = false;
}

void displayFunc()
{
  // This function performs the actual rendering.

  // First, clear the screen.
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set up the camera position, focus point, and the up vector.
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.LookAt(0.0, 3.5, 4.0,
                0.0, 0.0, 0.0,
                0.0, 1.0, 0.0);

  // In here, you can do additional modeling on the object, such as performing translations, rotations and scales.
  // x, y, z has to be a unit vector

  // Rotation about x axis
  matrix.Rotate(terrainRotate[0], 1.0, 0.0, 0.0);

  // Rotation about y axis
  matrix.Rotate(terrainRotate[1], 0.0, 1.0, 0.0);

  // Rotation about z axis
  matrix.Rotate(terrainRotate[2], 0.0, 0.0, 1.0);

  // Scale in the x direction
  matrix.Scale(terrainScale[0], terrainScale[1], terrainScale[2]);

  matrix.Translate(terrainTranslate[0], terrainTranslate[1], terrainTranslate[2]);

  // Read the current modelview and projection matrices from our helper class.
  // The matrices are only read here; nothing is actually communicated to OpenGL yet.
  float modelViewMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);

  // Upload the modelview and projection matrices to the GPU. Note that these are "uniform" variables.
  // Important: these matrices must be uploaded to *all* pipeline programs used.
  // In hw1, there is only one pipeline program, but in hw2 there will be several of them.
  // In such a case, you must separately upload to *each* pipeline program.
  // Important: do not make a typo in the variable name below; otherwise, the program will malfunction.
  pipelineProgram.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  pipelineProgram.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

  // Execute the rendering.
  // Bind the VAO that we want to render. Remember, one object = one VAO.
  switch (renderMode)
  {
  case POINTS:
    pointsVao.Bind();
    glDrawArrays(GL_POINTS, 0, numPointVertices); // Render the VAO, by rendering "numPointVertices", starting from vertex 0.
    break;

  case LINES:
    linesVao.Bind();
    glDrawArrays(GL_LINES, 0, numLineVertices);
    break;

  case TRIANGLES:
    trianglesVao.Bind();
    glDrawArrays(GL_TRIANGLES, 0, numTriangleVertices);
    break;

  case SMOOTH_TRIANGLES:
    smoothTrianglesVAO.Bind();
    glDrawArrays(GL_TRIANGLES, 0, numTriangleVertices);
    break;
  }

  // Swap the double-buffers.
  glutSwapBuffers();
}

void initScene(int argc, char *argv[])
{
  // Load the image from a jpeg disk file into main memory.
  std::unique_ptr<ImageIO> heightmapImage = std::make_unique<ImageIO>();
  if (heightmapImage->loadJPEG(argv[1]) != ImageIO::OK)
  {
    cout << "Error reading image " << argv[1] << "." << endl;
    exit(EXIT_FAILURE);
  }

  // Set the background color.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.

  // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
  glEnable(GL_DEPTH_TEST);

  // Create a pipeline program. This operation must be performed BEFORE we initialize any VAOs.
  // A pipeline program contains our shaders. Different pipeline programs may contain different shaders.
  // In this homework, we only have one set of shaders, and therefore, there is only one pipeline program.
  // In hw2, we will need to shade different objects with different shaders, and therefore, we will have
  // several pipeline programs (e.g., one for the rails, one for the ground/sky, etc.).
  // Load and set up the pipeline program, including its shaders.
  if (pipelineProgram.BuildShadersFromFiles(shaderBasePath, "vertexShader.glsl", "fragmentShader.glsl") != 0)
  {
    cout << "Failed to build the pipeline program." << endl;
    throw 1;
  }
  cout << "Successfully built the pipeline program." << endl;

  // Bind the pipeline program that we just created.
  // The purpose of binding a pipeline program is to activate the shaders that it contains, i.e.,
  // any object rendered from that point on, will use those shaders.
  // When the application starts, no pipeline program is bound, which means that rendering is not set up.
  // So, at some point (such as below), we need to bind a pipeline program.
  // From that point on, exactly one pipeline program is bound at any moment of time.
  pipelineProgram.Bind();

  const int h = heightmapImage->getHeight();
  const int w = heightmapImage->getWidth();

  const float resolution = h / 4;
  const float xBias = w / 2;
  const float zBias = h / 2;

  /*****************************************************************
                                POINTS
  *****************************************************************/

  // (x,y,z) coordinates for each vertex
  numPointVertices = h * w;
  std::unique_ptr<float[]> pointPositions = std::make_unique<float[]>(numPointVertices * 3);
  std::unique_ptr<float[]> pointColors = std::make_unique<float[]>(numPointVertices * 4);

  int vIndex = 0;
  int cIndex = 0;

  // setting point positions
  for (int y = 0; y < h; ++y)
  {
    for (int x = 0; x < w; ++x)
    {
      unsigned char pixel = heightmapImage->getPixel(x, y, 0);
      Vertex vertex = createVertex(xBias, zBias, resolution, pixel, x, y);
      addVertexToVBO(pointPositions, pointColors, vIndex, cIndex, vertex);
    }
  }

  /*****************************************************************
                                LINES
  *****************************************************************/

  // creating line buffers
  numLineVertices = 2 * ((h * (w - 1)) + (w * (h - 1))); // deduced from hw1-helper slides
  std::unique_ptr<float[]> lineVertices = std::make_unique<float[]>(numLineVertices * 3);
  std::unique_ptr<float[]> lineColors = std::make_unique<float[]>(numLineVertices * 4);

  // **IMPORTANT**
  vIndex = 0;
  cIndex = 0;

  // setting vertices
  for (int y = 0; y < h; ++y)
  {
    for (int x = 0; x < w; ++x)
    {
      // create first point
      Vertex vertex1 = createVertex(xBias, zBias, resolution, heightmapImage->getPixel(x, y, 0), x, y);

      // create line in y-direction if not out of bounds
      if (y < h - 1)
      {
        Vertex vertex2 = createVertex(xBias, zBias, resolution, heightmapImage->getPixel(x, y + 1, 0), x, y + 1);
        addLineToVBO(lineVertices, lineColors, vIndex, cIndex, vertex1, vertex2);
      }

      // create line in x-direction if not out of bounds
      if (x < w - 1)
      {
        Vertex vertex2 = createVertex(xBias, zBias, resolution, heightmapImage->getPixel(x + 1, y, 0), x + 1, y);
        addLineToVBO(lineVertices, lineColors, vIndex, cIndex, vertex1, vertex2);
      }
    }
  }

  /*****************************************************************
                                TRIANGLES
  *****************************************************************/

  // creating triangle buffers
  numTriangleVertices = 6 * (h - 1) * (w - 1); // deduced from hw1-helper slides
  std::unique_ptr<float[]> triangleVertices = std::make_unique<float[]>(numTriangleVertices * 3);
  std::unique_ptr<float[]> triangleColors = std::make_unique<float[]>(numTriangleVertices * 4);

  // **IMPORTANT**
  vIndex = 0;
  cIndex = 0;

  // setting vertices
  for (int y = 0; y < h - 1; ++y)
  {
    for (int x = 0; x < w - 1; ++x)
    {
      // bottom triangle
      Vertex vertex1 = createVertex(xBias, zBias, resolution, heightmapImage->getPixel(x, y, 0), x, y);
      Vertex vertex2 = createVertex(xBias, zBias, resolution, heightmapImage->getPixel(x + 1, y, 0), x + 1, y);
      Vertex vertex3 = createVertex(xBias, zBias, resolution, heightmapImage->getPixel(x + 1, y + 1, 0), x + 1, y + 1);

      // top triangleprintVertex(centerVertices, "center", 100);
      Vertex vertex4 = createVertex(xBias, zBias, resolution, heightmapImage->getPixel(x, y + 1, 0), x, y + 1);
      Vertex vertex5 = createVertex(xBias, zBias, resolution, heightmapImage->getPixel(x + 1, y + 1, 0), x + 1, y + 1);

      addTriangleToVBO(triangleVertices, triangleColors, vIndex, cIndex, vertex1, vertex2, vertex3);
      addTriangleToVBO(triangleVertices, triangleColors, vIndex, cIndex, vertex1, vertex4, vertex5);
    }
  }

  /*****************************************************************
                            SMOOTH TRIANGLES
  *****************************************************************/

  std::unique_ptr<float[]> centerVertices = std::make_unique<float[]>(numTriangleVertices * 3);
  std::unique_ptr<float[]> leftVertices = std::make_unique<float[]>(numTriangleVertices * 3);
  std::unique_ptr<float[]> rightVertices = std::make_unique<float[]>(numTriangleVertices * 3);
  std::unique_ptr<float[]> upVertices = std::make_unique<float[]>(numTriangleVertices * 3);
  std::unique_ptr<float[]> downVertices = std::make_unique<float[]>(numTriangleVertices * 3);
  // we don't need another color VBO because we can re-use the one from the other triangles

  vector<float *> vbos = {centerVertices.get(), leftVertices.get(), rightVertices.get(), upVertices.get(), downVertices.get()};

  vIndex = 0;
  cIndex = 0;

  for (int y = 0; y < h - 1; ++y)
  {
    for (int x = 0; x < w - 1; ++x)
    {
      // returns { center, left, right, up, down }
      vector<Vertex> vertices1 = createVertexAndNeighbors(heightmapImage, xBias, zBias, resolution, x, y, h, w);
      vector<Vertex> vertices2 = createVertexAndNeighbors(heightmapImage, xBias, zBias, resolution, x + 1, y, h, w);
      vector<Vertex> vertices3 = createVertexAndNeighbors(heightmapImage, xBias, zBias, resolution, x + 1, y + 1, h, w);

      vector<Vertex> vertices4 = createVertexAndNeighbors(heightmapImage, xBias, zBias, resolution, x, y + 1, h, w);
      vector<Vertex> vertices5 = createVertexAndNeighbors(heightmapImage, xBias, zBias, resolution, x + 1, y + 1, h, w);

      addTriangleVerticesToVBO(vbos, vIndex, vertices1, vertices2, vertices3);
      addTriangleVerticesToVBO(vbos, vIndex, vertices1, vertices4, vertices5);
    }
  }

  int i = 100;
  printVertex(centerVertices, "center", i);
  printVertex(leftVertices, "left", i);
  printVertex(rightVertices, "right", i);
  printVertex(upVertices, "up", i);
  printVertex(downVertices, "down", i);

  // Create the VBOs.
  // We make a separate VBO for vertices and colors.
  // This operation must be performed BEFORE we initialize any VAOs.
  pointsVboVertices.Gen(numPointVertices, 3, pointPositions.get(), GL_STATIC_DRAW); // 3 values per position
  pointsVboColors.Gen(numPointVertices, 4, pointColors.get(), GL_STATIC_DRAW);      // 4 values per color

  linesVboVertices.Gen(numLineVertices, 3, lineVertices.get(), GL_STATIC_DRAW);
  linesVboColors.Gen(numLineVertices, 4, lineColors.get(), GL_STATIC_DRAW);

  trianglesVboVertices.Gen(numTriangleVertices, 3, triangleVertices.get(), GL_STATIC_DRAW);
  trianglesVboColors.Gen(numTriangleVertices, 4, triangleColors.get(), GL_STATIC_DRAW);

  // smooth triangles
  centerSmoothTriangesVBO.Gen(numTriangleVertices, 3, centerVertices.get(), GL_STATIC_DRAW);
  leftSmoothTriangesVBO.Gen(numTriangleVertices, 3, leftVertices.get(), GL_STATIC_DRAW);
  rightSmoothTriangesVBO.Gen(numTriangleVertices, 3, rightVertices.get(), GL_STATIC_DRAW);
  upSmoothTriangesVBO.Gen(numTriangleVertices, 3, upVertices.get(), GL_STATIC_DRAW);
  downSmoothTriangesVBO.Gen(numTriangleVertices, 3, downVertices.get(), GL_STATIC_DRAW);

  // Create the VAOs. There is a single VAO in this example.
  // Important: this code must be executed AFTER we created our pipeline program, and AFTER we set up our VBOs.
  // A VAO contains the geometry for a single object. There should be one VAO per object.
  // In this homework, "geometry" means vertex positions and colors. In homework 2, it will also include
  // vertex normal and vertex texture coordinates for texture mapping.
  pointsVao.Gen();
  linesVao.Gen();
  trianglesVao.Gen();
  smoothTrianglesVAO.Gen();

  // Set up the relationship between the "position" shader variable and the VAO.
  // Important: any typo in the shader variable name will lead to malfunction.
  pointsVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &pointsVboVertices, "position");
  pointsVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &pointsVboColors, "color");

  linesVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &linesVboVertices, "position");
  linesVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &linesVboColors, "color");

  trianglesVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &trianglesVboVertices, "position");
  trianglesVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &trianglesVboColors, "color");

  // smooth triangles. NOTE: "center" --> "position"
  smoothTrianglesVAO.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &centerSmoothTriangesVBO, "position");
  smoothTrianglesVAO.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &leftSmoothTriangesVBO, "left");
  smoothTrianglesVAO.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &rightSmoothTriangesVBO, "right");
  smoothTrianglesVAO.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &upSmoothTriangesVBO, "up");
  smoothTrianglesVAO.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &downSmoothTriangesVBO, "down");
  smoothTrianglesVAO.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &trianglesVboColors, "color");

  // Check for any OpenGL errors.
  std::cout << "GL error status is: " << glGetError() << std::endl;

  // for animation
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    printf("Usage: %s <spline file>\n", argv[0]);
    exit(0);
  }

  // Load spline from the provided filename.
  loadSpline(argv[1]);

  printf("Loaded spline with %d control point(s).\n", spline.numControlPoints);

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc, argv);

  cout << "Initializing OpenGL..." << endl;

#ifdef __APPLE__
  glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#else
  glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
#endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

#ifdef __APPLE__
  // This is needed on recent Mac OS X versions to correctly display the window.
  glutReshapeWindow(windowWidth - 1, windowHeight - 1);
#endif

  // Tells GLUT to use a particular display function to redraw.
  glutDisplayFunc(displayFunc);
  // Perform animation inside idleFunc.
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);
  // callback for releasing the keys on the keyboard
  glutKeyboardUpFunc(keyboardUpFunc);

// init glew
#ifdef __APPLE__
  // nothing is needed on Apple
#else
  // Windows, Linux
  GLint result = glewInit();
  if (result != GLEW_OK)
  {
    cout << "error: " << glewGetErrorString(result) << endl;
    exit(EXIT_FAILURE);
  }
#endif

  // Perform the initialization.
  initScene(argc, argv);

  // Sink forever into the GLUT loop.
  glutMainLoop();
}

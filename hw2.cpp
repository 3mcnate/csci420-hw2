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

// Number of vertices in the spline
int numSplineVertices;

// CSCI 420 helper classes.
OpenGLMatrix matrix;
PipelineProgram pipelineProgram;

// spline
VBO splinesVboVertices;
VBO splinesVboColors;
VAO splinesVao;

// axis for debugging
VBO axisVboVertices;
VBO axisVboColors;
VAO axisVao;

float upos = 0;
int currPoint = 0;

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
  }
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

  // cout << "upos " << upos << endl;
  if (upos >= 1)
  {
    upos = 0;
    currPoint = (currPoint + 1) % spline.numControlPoints;
  }

  // increment upos so we move forward
  upos += 0.01;

  float *R = calcMC(currPoint);

  // calculate position
  vector<float> posMatrix = {upos * upos * upos, upos * upos, upos, 1};
  float position[3];
  MultiplyMatrices(1, 4, 3, posMatrix.data(), R, position);

  // calculate tangent
  vector<float> tangentMatrix = {3 * upos * upos, 2 * upos, 1, 0};
  float tangent[3];
  MultiplyMatrices(1, 4, 3, tangentMatrix.data(), R, tangent);
  normalizeVector(tangent);

  // cout << "\nPosition: " << position[0] << " " << position[1] << " " << position[2] << endl;
  // cout << "Tangent: " << tangent[0] << " " << tangent[1] << " " << tangent[2] << endl;

  // important :)
  delete[] R;

  // Set up the camera position, focus point, and the up vector.
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  // bux fix: needed to add tangent to position to get correct direction
  matrix.LookAt(position[0], position[1], position[2],
                position[0] + tangent[0], position[1] + tangent[1], position[2] + tangent[2],
                0.0, 1.0, 0.0);

  // matrix.LookAt(0, 3, 5,
  //               0, 0, -1,
  //               0, 1, 0);

#define CREATIVE_MODE
#ifdef CREATIVE_MODE
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
#endif

  float modelViewMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);

  pipelineProgram.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  pipelineProgram.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

  // Execute the rendering.
  // Bind the VAO that we want to render. Remember, one object = one VAO.
  splinesVao.Bind();
  glDrawArrays(GL_LINES, 0, numSplineVertices);

  axisVao.Bind();
  glDrawArrays(GL_LINES, 0, 6);

  // Swap the double-buffers.
  glutSwapBuffers();
}

void initScene(int argc, char *argv[])
{
  // Set the background color.
  glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // Black color.

  // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
  glEnable(GL_DEPTH_TEST);

  if (pipelineProgram.BuildShadersFromFiles(shaderBasePath, "vertexShader.glsl", "fragmentShader.glsl") != 0)
  {
    cout << "Failed to build the pipeline program." << endl;
    throw 1;
  }
  cout << "Successfully built the pipeline program." << endl;

  pipelineProgram.Bind();

  /*****************************************************************
                              MILESTONE
  *****************************************************************/

  // creating line buffers
  vector<float> splineVertices;
  vector<float> splineColors;

  vector<float> splineNormals;
  vector<float> splineBinormals;

  float arbitraryV[3] = {1,
                         0,
                         0};

  // for each set of 4 control points, create a curve
  for (int i = 0; i < spline.numControlPoints - 3; ++i)
  {
    cout << "numControlPoints: " << spline.numControlPoints << endl;
    cout << "calculating spline matrix starting at point " << i << endl;
    float *R = calcMC(i, true);

    // draw the spline
    for (float u = 0.0f; u < 0.9999f; u += 0.001f)
    {
      // calculate position
      vector<float> posMatrix = {u * u * u, u * u, u, 1};
      float point[3];
      MultiplyMatrices(1, 4, 3, posMatrix.data(), R, point);

      // calculate tangent
      vector<float> tangentMatrix = {3 * u * u, 2 * u, 1, 0};
      float tangent[3];
      MultiplyMatrices(1, 4, 3, posMatrix.data(), R, tangent);

      // add vertices to VBO
      // Note: we must add each vertex except the first and last twice to connect the lines.
      bool addOnlyOnce = (i == 0 && u == 0.0f) || (i == (spline.numControlPoints - 3) && (u >= 0.9985f));

      for (int j = 0; j < (addOnlyOnce ? 1 : 2); ++j)
      {
        splineVertices.push_back(point[0]);
        splineVertices.push_back(point[1]);
        splineVertices.push_back(point[2]);
      }

      // add colors to VBO
      for (int j = 0; j < (addOnlyOnce ? 4 : 8); ++j)
        splineColors.push_back(1.0f);
    }

    delete[] R;
  }

  numSplineVertices = (int)splineVertices.size() / 3;
  splinesVboVertices.Gen(numSplineVertices, 3, splineVertices.data(), GL_STATIC_DRAW); // 3 values per position
  splinesVboColors.Gen(numSplineVertices, 4, splineColors.data(), GL_STATIC_DRAW);     // 4 values per color

  splinesVao.Gen();
  splinesVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &splinesVboVertices, "position");
  splinesVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &splinesVboColors, "color");

  // debugging
  float axis_length = 4;
  vector<float> axisVertices = {-axis_length, 0, 0,
                                axis_length, 0, 0,
                                0, axis_length, 0,
                                0, -axis_length, 0,
                                0, 0, axis_length,
                                0, 0, -axis_length};

  vector<float> axisColors = {1, 0, 0, 1,
                              1, 0, 0, 1,
                              0, 1, 0, 1,
                              0, 1, 0, 1,
                              0, 0, 1, 1,
                              0, 0, 1, 1};

  axisVboVertices.Gen(6, 3, axisVertices.data(), GL_STATIC_DRAW);
  axisVboColors.Gen(6, 3, axisColors.data(), GL_STATIC_DRAW);

  axisVao.Gen();
  axisVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &axisVboVertices, "position");
  axisVao.ConnectPipelineProgramAndVBOAndShaderVariable(&pipelineProgram, &axisVboColors, "color");

  // Check for any OpenGL errors.
  std::cout << "GL error status is: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc < 2)
  {
    printf("Usage: %s <spline file>\n", argv[0]);
    return 0;
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

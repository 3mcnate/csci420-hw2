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
char shaderBasePath[1024] = "shaders";
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

// CSCI 420 helper classes.
OpenGLMatrix matrix;
PipelineProgram milestonePipelineProgram;
PipelineProgram texturePipelineProgram;

// Number of vertices in the spline
int numSplineVertices;
int numRailVertices;
int numGroundVertices;

// ground texture
GLuint texHandle;
VBO groundVerticesVBO;
VBO groundUVMapVBO;
VAO groundVAO;

// spline
VBO splinesVboVertices;
VBO splinesVboColors;
VAO splinesVao;

// track
VBO railVerticesVBO;
VBO railNormalsVBO;
VAO railVAO;

// axis for debugging
VBO axisVboVertices;
VBO axisVboColors;
VAO axisVao;

// for animating the motion
vector<Point> positions;
vector<Point> tangents;
vector<Point> binormals;

int currPos = 0;

// determined through experimentation
float La = 1.3;
float Ld = 1;
float Ls = 0.3;
float ka = 0.2;
float kd = 0.7;
float ks = 1.1;
float aph = 64.0;

int counter = 0;

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
  int modulo = 2;
  if (currPos % modulo == 0 && counter < 1000)
  {
    char buf[32];
    snprintf(buf, 32, "animation/%03d.jpg", counter);
    counter++;
    saveScreenshot(buf);
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
  const float humanFieldOfView = 85.0f;
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
  case 27: // ESC key
    cout << "La: " << La << endl;
    cout << "Ld: " << Ld << endl;
    cout << "Ls: " << Ls << endl;
    cout << "ka: " << ka << endl;
    cout << "kd: " << kd << endl;
    cout << "ks: " << ks << endl;
    cout << "aph: " << aph << endl;
    exit(0); // exit the program
    break;

  case ' ':
    cout << "You pressed the spacebar." << endl;
    break;

  case 'x':
    // Take a screenshot.
    saveScreenshot("screenshot.jpg");
    break;

  case 'q':
    La += 0.1;
    break;
  case 'a':
    La -= 0.1;
    break;

  case 'w':
    Ld += 0.1;
    break;
  case 's':
    Ld -= 0.1;
    break;

  case 'e':
    Ls += 0.1;
    break;
  case 'd':
    Ls -= 0.1;
    break;

  case 'r':
    ka += 0.1;
    break;
  case 'f':
    ka -= 0.1;
    break;

  case 't':
    kd += 0.1;
    break;
  case 'g':
    kd -= 0.1;
    break;

  case 'y':
    ks += 0.1;
    break;
  case 'h':
    ks -= 0.1;
    break;

  case 'u':
    aph += 0.1;
    break;
  case 'j':
    aph -= 0.1;
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

  Point &pos = positions[currPos];
  Point &tangent = tangents[currPos];
  Point &binormal = binormals[currPos];

  Point eye = pos + 1.3 * binormal;
  Point focus = eye + tangent;

  // Set up the camera position, focus point, and the up vector.
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.LookAt(eye.x, eye.y, eye.z,
                focus.x, focus.y, focus.z,
                binormal.x, binormal.y, binormal.z);

  // move forward
  currPos = (currPos + 1) % positions.size();

  float modelViewMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);

  float normalMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetNormalMatrix(normalMatrix);

  float lightDirection[4] = {0, 1, -1, 0}; // should the last coordinate be 1.0?
  normalizeVector(lightDirection);
  float viewLightDirection[4];
  MultiplyMatrices(4, 4, 1, modelViewMatrix, lightDirection, viewLightDirection);

  // stupid ass order
  milestonePipelineProgram.Bind();
  milestonePipelineProgram.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  milestonePipelineProgram.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
  milestonePipelineProgram.SetUniformVariableMatrix4fv("normalMatrix", GL_FALSE, normalMatrix);

  GLuint milestoneProgram_h = milestonePipelineProgram.GetProgramHandle();

  setUniform3f(milestoneProgram_h, "viewLightDirection", viewLightDirection[0], viewLightDirection[1], viewLightDirection[2]);
  setUniform4f(milestoneProgram_h, "La", La, La, La, 1);
  setUniform4f(milestoneProgram_h, "Ld", Ld, Ld, Ld, 1);
  setUniform4f(milestoneProgram_h, "Ls", Ls, Ls, Ls, 1);
  setUniform4f(milestoneProgram_h, "ka", ka, ka, ka, 1);
  setUniform4f(milestoneProgram_h, "kd", kd, kd, kd, 1);
  setUniform4f(milestoneProgram_h, "ks", ks, ks, ks, 1);
  milestonePipelineProgram.SetUniformVariablef("alpha", aph);

  railVAO.Bind();
  glDrawArrays(GL_TRIANGLES, 0, numRailVertices);

  // make sure to switch to the texture pipeline program
  texturePipelineProgram.Bind();
  texturePipelineProgram.SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  texturePipelineProgram.SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);
  setTextureUnit(texturePipelineProgram.GetProgramHandle(), GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, texHandle); // select the texture to use (“texHandle” was generated by glGenTextures)

  groundVAO.Bind();
  glDrawArrays(GL_TRIANGLES, 0, numGroundVertices);

  // Swap the double-buffers.
  glutSwapBuffers();
}

void initScene(int argc, char *argv[])
{
  // Set the background color.
  // sky blue RGB: (3, 165, 252)
  glClearColor(3.0 / 255.0, 165.0 / 255.0, 252.0 / 255.0, 1.0f);

  // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
  glEnable(GL_DEPTH_TEST);

  if (milestonePipelineProgram.BuildShadersFromFiles(shaderBasePath, "phongVertexShader.glsl", "phongFragmentShader.glsl") != 0)
  {
    cout << "Failed to build the milestone pipeline program." << endl;
    throw 1;
  }
  cout << "Successfully built the milestone pipeline program." << endl;

  /*****************************************************************
                            LVL 4: TEXTURE
  *****************************************************************/

  if (texturePipelineProgram.BuildShadersFromFiles(shaderBasePath, "textureVertexShader.glsl", "textureFragmentShader.glsl"))
  {
    cout << "Failed to build the texture pipeline program." << endl;
    throw 1;
  }
  cout << "Successfully built the texture pipeline program." << endl;

  glGenTextures(1, &texHandle);
  if (initTexture(argv[2], texHandle))
  {
    cout << "Error loading the texture image " << argv[2] << endl;
    throw 1;
  }

  vector<float> groundVertices;
  vector<float> groundUVMap;

  const float textureSize = 40;
  const float depth = 20;
  TexturePoint p1(textureSize, -depth, textureSize, 1, 1);
  TexturePoint p2(textureSize, -depth, -textureSize, 1, 0);
  TexturePoint p3(-textureSize, -depth, -textureSize, 0, 0);
  TexturePoint p4(-textureSize, -depth, textureSize, 0, 1);

  addTextureTriangleToVectors(p1, p2, p3, groundVertices, groundUVMap);
  addTextureTriangleToVectors(p1, p3, p4, groundVertices, groundUVMap);

  numGroundVertices = (int)groundVertices.size() / 3;
  groundVerticesVBO.Gen(numGroundVertices, 3, groundVertices.data(), GL_STATIC_DRAW);
  groundUVMapVBO.Gen(numGroundVertices, 2, groundUVMap.data(), GL_STATIC_DRAW);

  groundVAO.Gen();
  groundVAO.ConnectPipelineProgramAndVBOAndShaderVariable(&texturePipelineProgram, &groundVerticesVBO, "position");
  groundVAO.ConnectPipelineProgramAndVBOAndShaderVariable(&texturePipelineProgram, &groundUVMapVBO, "texCoord");

  /*****************************************************************
                              MILESTONE
  *****************************************************************/

  // creating line buffers
  vector<float> splineVertices;
  vector<float> splineColors;

  // for each set of 4 control points, create a curve
  for (int i = 0; i < spline.numControlPoints - 3; ++i)
  {
    // cout << "numControlPoints: " << spline.numControlPoints << endl;
    // cout << "calculating spline matrix starting at point " << i << endl;
    float *R = calcMC(i);

    // draw the spline
    for (float u = 0.0f; u < 0.9999f; u += 0.001f)
    {
      // calculate position
      Point point = calculatePosition(u, R);

      // add vertices to VBO
      // Note: we must add each vertex except the first and last twice to connect the lines.
      bool addOnlyOnce = (i == 0 && u == 0.0f) || (i == (spline.numControlPoints - 3) && (u >= 0.9985f));

      addPointToVector(point, splineVertices);
      if (!addOnlyOnce)
        addPointToVector(point, splineVertices);

      // add colors to VBO
      for (int j = 0; j < (addOnlyOnce ? 4 : 8); ++j)
        splineColors.push_back(1.0f);
    }

    delete[] R;
  }

  // connect all the shit together
  numSplineVertices = (int)splineVertices.size() / 3;
  splinesVboVertices.Gen(numSplineVertices, 3, splineVertices.data(), GL_STATIC_DRAW); // 3 values per position
  splinesVboColors.Gen(numSplineVertices, 4, splineColors.data(), GL_STATIC_DRAW);     // 4 values per color

  splinesVao.Gen();
  splinesVao.ConnectPipelineProgramAndVBOAndShaderVariable(&milestonePipelineProgram, &splinesVboVertices, "position");
  splinesVao.ConnectPipelineProgramAndVBOAndShaderVariable(&milestonePipelineProgram, &splinesVboColors, "color");

  /*****************************************************************
                                LEVEL 3
  *****************************************************************/

  vector<float> railVertices;
  vector<float> railColors;
  vector<float> railNormals;

  float arbitraryV[3] = {1,
                         0,
                         0};

  vector<Point> prev4;
  Point prevB0(0, 0, 0);

  // for each set of 4 control points, create a curve
  for (int i = 0; i < spline.numControlPoints - 3; ++i)
  {
    float *R = calcMC(i);

    // calculate vertices
    for (float u = 0.0f; u < 0.9999f; u += 0.01f)
    {
      bool isFirstPoint = i == 0 && u == 0.0f;

      // calculate position
      Point P0 = calculatePosition(u, R);

      // calculate tangent
      Point T0 = calculateTangent(u, R).normalize();

      // initialization for the first point
      Point N0(0, 0, 0);
      if (isFirstPoint)
        N0 = crossProduct(T0, arbitraryV).normalize();
      else
        N0 = crossProduct(prevB0, T0).normalize();

      // calculate binormal
      Point B0 = crossProduct(T0, N0).normalize();

      // Now calculate the necessary vertices
      float alpha = 0.1;
      Point V0 = P0 + alpha * (-N0 + B0);
      Point V1 = P0 + alpha * (N0 + B0);
      Point V2 = P0 + alpha * (N0 + -B0);
      Point V3 = P0 + alpha * (-N0 + -B0);

      // first triangles form the end of the rail
      if (isFirstPoint)
      {
        addTriangleToVector(V0, V1, V2, railVertices);
        addTriangleToVector(V0, V3, V2, railVertices);

        for (int i = 0; i < 6; i++)
          addColorToVector(T0, railColors);
      }
      else
      {
        // form rail segment
        Point &V4 = prev4[0];
        Point &V5 = prev4[1];
        Point &V6 = prev4[2];
        Point &V7 = prev4[3];

        addTriangleToVector(V0, V1, V4, railVertices);
        addTriangleToVector(V1, V4, V5, railVertices);
        addTriangleNormalToVector(B0, railNormals);
        addTriangleNormalToVector(B0, railNormals);

        addTriangleToVector(V1, V2, V5, railVertices);
        addTriangleToVector(V2, V5, V6, railVertices);
        addTriangleNormalToVector(N0, railNormals);
        addTriangleNormalToVector(N0, railNormals);

        addTriangleToVector(V2, V3, V6, railVertices);
        addTriangleToVector(V3, V6, V7, railVertices);
        addTriangleNormalToVector(-B0, railNormals);
        addTriangleNormalToVector(-B0, railNormals);

        addTriangleToVector(V3, V0, V7, railVertices);
        addTriangleToVector(V0, V7, V4, railVertices);
        addTriangleNormalToVector(-N0, railNormals);
        addTriangleNormalToVector(-N0, railNormals);
      }

      // save position, tangent, and normal for rollercoaster motions
      positions.push_back(P0);
      tangents.push_back(T0);
      binormals.push_back(B0);

      // Save the vertices for the next calculation
      prev4.clear();
      prev4.push_back(V0);
      prev4.push_back(V1);
      prev4.push_back(V2);
      prev4.push_back(V3);

      // save B0 for the next calculation
      prevB0 = B0;
    }

    delete[] R;
  }

  // connect all this shit again
  numRailVertices = (int)railVertices.size() / 3;
  railVerticesVBO.Gen(numRailVertices, 3, railVertices.data(), GL_STATIC_DRAW);
  railNormalsVBO.Gen(numRailVertices, 3, railNormals.data(), GL_STATIC_DRAW);
  // NOTE: Normals are being used for colors  ^^^

  railVAO.Gen();
  railVAO.ConnectPipelineProgramAndVBOAndShaderVariable(&milestonePipelineProgram, &railVerticesVBO, "position");
  railVAO.ConnectPipelineProgramAndVBOAndShaderVariable(&milestonePipelineProgram, &railNormalsVBO, "normal");

  /*****************************************************************
                            AXIS DEBUGGING
  *****************************************************************/

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
  axisVao.ConnectPipelineProgramAndVBOAndShaderVariable(&milestonePipelineProgram, &axisVboVertices, "position");
  axisVao.ConnectPipelineProgramAndVBOAndShaderVariable(&milestonePipelineProgram, &axisVboColors, "color");

  // Check for any OpenGL errors.
  std::cout << "GL error status is: " << glGetError() << std::endl;
}

int main(int argc, char *argv[])
{
  if (argc < 3)
  {
    printf("Usage: %s <spline file> <texture file>\n", argv[0]);
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

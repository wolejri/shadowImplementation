
/* Athabasca University
   Comp390 - Introduction to Computer Graphics
   Final Project Option 3
   @author: Samuel Idowu
   @student#: 3458622
   @date: March 17, 2022
   Program: The purpose of this program is to deliver a shadow scene that includes
            * at least one reflective object
            * a computed ground (e.g., a black and white check board);
            * at least two light sources.
            * at least two shadows (depending on the number of light sources) for each reflective object
            * I have explored the use of OpenGL STENCIL'S BUFFER AND ITS POLYGON OFFSET FUNCIONALITY to implement the scene. In addition to this, I'll be using the stb_image library to render my textured objects.

 */



#ifdef __APPLE__
/* Defined before OpenGL and GLUT includes to avoid deprecation messages */
#define GL_SILENCE_DEPRECATION
#include <OpenGL/gl.h>
#include <GLUT/glut.h>
#else
#include <GL/gl.h>
#include <GL/glut.h>
#endif
#include <iostream>
#include <unistd.h>
#include <fstream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include "math.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;


/* Variable controlling various rendering modes. */
static int stencilReflection = 1, stencilShadow = 1, offsetShadow = 1;
static int renderShadow = 1, renderDinosaur = 1, renderReflection = 1;
static int linearFiltering = 0, useMipmaps = 0, useTexture = 1;
static int animation = 1;
static GLboolean lightSwitch = GL_TRUE;
static int directionalLight = 1;
static int forceExtension = 0;
static GLfloat floorPlane[4];
static GLfloat floorShadow[4][4];
unsigned int texture;
unsigned int textureTwo;

/* Time varying or user-controled variables. */
static float jump = 0.0;
static float lightAngle = 0.0, lightHeight = 25;
GLfloat angle = 60;   /* in degrees */
GLfloat angle2 = 30;   /* in degrees */

int moving, startx, starty;
int lightMoving = 0, lightStartX, lightStartY;

enum {
  MISSING, EXTENSION, ONE_DOT_ONE
};

enum {
  F_NONE, F_MOTION, F_LIGHT, F_TEXTURE, F_SHADOWS, F_REFLECTION, F_DINOSAUR,
  F_STENCIL_REFLECTION, F_STENCIL_SHADOW, F_OFFSET_SHADOW,
  F_POSITIONAL, F_DIRECTIONAL
};
enum {
  X, Y, Z, W
};
enum {
  A, B, C, D
};

/* Enumerants for refering to display lists. */
typedef enum {
  RESERVED, BODY_SIDE, BODY_EDGE, BODY_WHOLE, ARM_SIDE, ARM_EDGE, ARM_WHOLE,
  LEG_SIDE, LEG_EDGE, LEG_WHOLE, EYE_SIDE, EYE_EDGE, EYE_WHOLE
} displayLists;

int polygonOffsetVersion;

static GLdouble bodyWidth = 4.0;

/* * Specification for Dinosaur * */
static GLfloat body[][2] = { {0, 3}, {1, 1}, {5, 1}, {8, 4}, {10, 4}, {11, 5},
  {11, 11.5}, {13, 12}, {13, 13}, {10, 13.5}, {13, 14}, {13, 15}, {11, 16},
  {8, 16}, {7, 15}, {7, 13}, {8, 12}, {7, 11}, {6, 6}, {4, 3}, {3, 2},
  {1, 2} };

static GLfloat arm[][2] = { {8, 10}, {9, 9}, {10, 9}, {13, 8}, {14, 9}, {16, 9},
  {15, 9.5}, {16, 10}, {15, 10}, {15.5, 11}, {14.5, 10}, {14, 11}, {14, 10},
  {13, 9}, {11, 11}, {9, 11} };

static GLfloat leg[][2] = { {8, 6}, {8, 4}, {9, 3}, {9, 2}, {8, 1}, {8, 0.5}, {9, 0},
  {12, 0}, {10, 1}, {10, 2}, {12, 4}, {11, 6}, {10, 7}, {9, 7} };

static GLfloat eye[][2] = { {8.75, 15}, {9, 14.7}, {9.6, 14.7}, {10.1, 15},
  {9.6, 15.25}, {9, 15.25} };

static GLfloat lightPosition[4];
static GLfloat lightColor[] = {0.8, 1.0, 0.8, 1.0}; /* green-tinted */
static GLfloat skinColor[] = {0.1, 0.1, 1.0, 1.0}, eyeColor[] = {0.9, 0.1, 0.1, 1.0};
static GLfloat floorVertices[4][3] = {
  { -20.0, 0.0, 20.0 },
  { 20.0, 0.0, 20.0 },
  { 20.0, 0.0, -20.0 },
  { -20.0, 0.0, -20.0 },
};

/* Specification for another solid, a cube*/
GLfloat n[6][3] = {  /* Normals for the 6 faces of a cube. */
  {-1.0, 0.0, 0.0}, {0.0, 1.0, 0.0}, {1.0, 0.0, 0.0},
  {0.0, -1.0, 0.0}, {0.0, 0.0, 1.0}, {0.0, 0.0, -1.0} };
GLint faces[6][4] = {  /* Vertex indices for the 6 faces of a cube. */
  {0, 1, 2, 3}, {3, 2, 6, 7}, {7, 6, 5, 4},
  {4, 5, 1, 0}, {5, 6, 2, 1}, {7, 4, 0, 3} };
GLfloat v[8][3];  /* Will be filled in with X,Y,Z vertexes. */




/* Create a matrix that will project the desired shadow. */
void shadowMatrix(GLfloat shadowMat[4][4], GLfloat groundplane[4],GLfloat lightpos[4])
{
  GLfloat dot;

  /* Find dot product between light position vector and ground plane normal. */
  dot = groundplane[X] * lightpos[X] +
    groundplane[Y] * lightpos[Y] +
    groundplane[Z] * lightpos[Z] +
    groundplane[W] * lightpos[W];

  shadowMat[0][0] = dot - lightpos[X] * groundplane[X];
  shadowMat[1][0] = 0.f - lightpos[X] * groundplane[Y];
  shadowMat[2][0] = 0.f - lightpos[X] * groundplane[Z];
  shadowMat[3][0] = 0.f - lightpos[X] * groundplane[W];

  shadowMat[X][1] = 0.f - lightpos[Y] * groundplane[X];
  shadowMat[1][1] = dot - lightpos[Y] * groundplane[Y];
  shadowMat[2][1] = 0.f - lightpos[Y] * groundplane[Z];
  shadowMat[3][1] = 0.f - lightpos[Y] * groundplane[W];

  shadowMat[X][2] = 0.f - lightpos[Z] * groundplane[X];
  shadowMat[1][2] = 0.f - lightpos[Z] * groundplane[Y];
  shadowMat[2][2] = dot - lightpos[Z] * groundplane[Z];
  shadowMat[3][2] = 0.f - lightpos[Z] * groundplane[W];

  shadowMat[X][3] = 0.f - lightpos[W] * groundplane[X];
  shadowMat[1][3] = 0.f - lightpos[W] * groundplane[Y];
  shadowMat[2][3] = 0.f - lightpos[W] * groundplane[Z];
  shadowMat[3][3] = dot - lightpos[W] * groundplane[W];

}

/* Find the plane equation given 3 points. */
void findPlane(GLfloat plane[4], GLfloat v0[3], GLfloat v1[3], GLfloat v2[3])
{
  GLfloat vec0[3], vec1[3];

  /* Need 2 vectors to find cross product. */
  vec0[X] = v1[X] - v0[X];
  vec0[Y] = v1[Y] - v0[Y];
  vec0[Z] = v1[Z] - v0[Z];

  vec1[X] = v2[X] - v0[X];
  vec1[Y] = v2[Y] - v0[Y];
  vec1[Z] = v2[Z] - v0[Z];

  /* find cross product to get A, B, and C of plane equation */
  plane[A] = vec0[Y] * vec1[Z] - vec0[Z] * vec1[Y];
  plane[B] = -(vec0[X] * vec1[Z] - vec0[Z] * vec1[X]);
  plane[C] = vec0[X] * vec1[Y] - vec0[Y] * vec1[X];

  plane[D] = -(plane[A] * v0[X] + plane[B] * v0[Y] + plane[C] * v0[Z]);
}


static void makeFloorTexture()
{
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    // load and generate the texture
    int width, height, nrChannels;
    
    string file = "checkeredGround.jpg";
    
    stbi_set_flip_vertically_on_load(true);
    unsigned char * imageData = stbi_load(file.c_str(), &width, &height, &nrChannels, 0);
    if (imageData)
    {
        cout << "success load image : width=" << width << ", height=" << height << endl;
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    else
    {
        std::cout << "Failed to load texture" << std::endl;
    }
    stbi_image_free(imageData);
}


/* Draw a checkered floor */
static void drawFloor()
{
      glDisable(GL_LIGHTING);

      if (useTexture) {
        glEnable(GL_TEXTURE_2D);
        glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
      }

      glBegin(GL_QUADS);
        glTexCoord2f(0.0, 0.0);
        glVertex3fv(floorVertices[0]);
        glTexCoord2f(1.0, .0);
        glVertex3fv(floorVertices[1]);
        glTexCoord2f(1.0, 1.0);
        glVertex3fv(floorVertices[2]);
        glTexCoord2f(0.0, 1.0);
        glVertex3fv(floorVertices[3]);
      glEnd();

      if (useTexture) {
        glDisable(GL_TEXTURE_2D);
      }

      glEnable(GL_LIGHTING);

}
/* This function draws a cube covered with textures */
static void makeCube()
{
    glDisable(GL_LIGHTING);
    
    
    if (useTexture) {
      glEnable(GL_TEXTURE_2D);
      glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
      glActiveTexture(GL_TEXTURE0);
      glBindTexture(GL_TEXTURE_2D, texture);
    }
    
    glPushMatrix();

      for (int i = 0; i < 6; i++) {
        glBegin(GL_QUADS);
        glNormal3fv(&n[i][0]);
          glTexCoord2f(0.0, 0.0);
        glVertex3fv(&v[faces[i][0]][0]);
          glTexCoord2f(1.0, 0.0);
        glVertex3fv(&v[faces[i][1]][0]);
          glTexCoord2f(1.0, 1.0);
        glVertex3fv(&v[faces[i][2]][0]);
          glTexCoord2f(0.0, 1.0);
        glVertex3fv(&v[faces[i][3]][0]);
        glEnd();
      }
    glPopMatrix();
    
    if (useTexture) {
      glDisable(GL_TEXTURE_2D);
    }
    glEnable(GL_LIGHTING);
    
}



void
extrudeSolidFromPolygon(GLfloat data[][2], unsigned int dataSize,
  GLdouble thickness, GLuint side, GLuint edge, GLuint whole)
{
  
  GLdouble dx, dy, len;
  int i;
  int count = dataSize / (2 * sizeof(GLfloat));

  glNewList(edge, GL_COMPILE);
  glShadeModel(GL_FLAT);  /* flat shade keeps angular hands from being "smoothed" */
  glBegin(GL_QUAD_STRIP);
  for (i = 0; i <= count; i++) {
    /* mod function handles closing the edge */
    glVertex3f(data[i % count][0], data[i % count][1], 0.0);
    glVertex3f(data[i % count][0], data[i % count][1], thickness);
      
    /* Calculate a unit normal by dividing by Euclidean
       distance.  */
    dx = data[(i + 1) % count][1] - data[i % count][1];
    dy = data[i % count][0] - data[(i + 1) % count][0];
    len = sqrt(dx * dx + dy * dy);
    glNormal3f(dx / len, dy / len, 0.0);
  }
  glEnd();
  glEndList();
  glNewList(whole, GL_COMPILE);
  glFrontFace(GL_CW);
  glCallList(edge);
  glNormal3f(0.0, 0.0, -1.0);  /* constant normal for side */
  glCallList(side);
  glPushMatrix();
  glTranslatef(0.0, 0.0, thickness);
  glFrontFace(GL_CCW);
  glNormal3f(0.0, 0.0, 1.0);  /* opposite normal for other side */
  glCallList(side);
  glPopMatrix();
  glEndList();
}


static void makeDinosaur()
{
  extrudeSolidFromPolygon(body, sizeof(body), bodyWidth,
    BODY_SIDE, BODY_EDGE, BODY_WHOLE);
  extrudeSolidFromPolygon(arm, sizeof(arm), bodyWidth / 4,
    ARM_SIDE, ARM_EDGE, ARM_WHOLE);
  extrudeSolidFromPolygon(leg, sizeof(leg), bodyWidth / 2,
    LEG_SIDE, LEG_EDGE, LEG_WHOLE);
  extrudeSolidFromPolygon(eye, sizeof(eye), bodyWidth + 0.2,
    EYE_SIDE, EYE_EDGE, EYE_WHOLE);
}

static void drawDinosaur()

{
  glPushMatrix();
  /* Translate the dinosaur to be at (0,8,0). */
  glTranslatef(-8, 0, -bodyWidth / 2);
  glTranslatef(0.0, jump, 0.0);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, skinColor);
  glCallList(BODY_WHOLE);
  glTranslatef(0.0, 0.0, bodyWidth);
  glCallList(ARM_WHOLE);
  glCallList(LEG_WHOLE);
  glTranslatef(0.0, 0.0, -bodyWidth - bodyWidth / 4);
  glCallList(ARM_WHOLE);
  glTranslatef(0.0, 0.0, -bodyWidth / 4);
  glCallList(LEG_WHOLE);
  glTranslatef(0.0, 0.0, bodyWidth / 2 - 0.1);
  glMaterialfv(GL_FRONT, GL_DIFFUSE, eyeColor);
  glCallList(EYE_WHOLE);
  glPopMatrix();
}

/* This function references two different lights for the scene */
void lights()
{
    glPushMatrix();
    glDisable(GL_LIGHTING);
    glColor3f(1.0, 1.0, 0.0);
    if (directionalLight) {
      /* Draw an arrowhead. */
      glDisable(GL_CULL_FACE);
      glTranslatef(lightPosition[0], lightPosition[1], lightPosition[2]);
      glRotatef(lightAngle * -180.0 / M_PI, 0, 1, 0);
      glRotatef(atan(lightHeight/12) * 180.0 / M_PI, 0, 0, 1);
      glBegin(GL_TRIANGLE_FAN);
       glVertex3f(0, 0, 0);
       glVertex3f(2, 1, 1);
       glVertex3f(2, -1, 1);
       glVertex3f(2, -1, -1);
       glVertex3f(2, 1, -1);
       glVertex3f(2, 1, 1);
      glEnd();
      /* Draw a white line from light direction. */
      glColor3f(1.0, 1.0, 1.0);
      glBegin(GL_LINES);
       glVertex3f(0, 0, 0);
       glVertex3f(5, 0, 0);
      glEnd();
      glEnable(GL_CULL_FACE);
    } else {
      /* Draw a yellow ball at the light source. */
      glTranslatef(lightPosition[0], lightPosition[1], lightPosition[2]);
      glutSolidSphere(1.0, 5, 5);
    }
    glEnable(GL_LIGHTING);
    glPopMatrix();
}

/* This method implements stencil buffer for the reflection and shadow of the objects */
static void scene()
{


  /* Clear; default stencil clears to zero. */
  if ((stencilReflection && renderReflection) || (stencilShadow && renderShadow)) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
  } else {
    /* Avoid clearing stencil when not using it. */
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  }

  /* Reposition the light source. */
  lightPosition[0] = 12*cos(lightAngle);
  lightPosition[1] = lightHeight;
  lightPosition[2] = 12*sin(lightAngle);
  if (directionalLight) {
    lightPosition[3] = 0.0;
  } else {
    lightPosition[3] = 1.0;
  }

  shadowMatrix(floorShadow, floorPlane, lightPosition);

  glPushMatrix();
    /* Perform scene rotations based on user mouse input. */
    glRotatef(angle2, 1.0, 0.0, 0.0);
    glRotatef(angle, 0.0, 1.0, 0.0);
     
    /* Tell GL new light source position. */
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

    if (renderReflection) {
      if (stencilReflection) {
        /* We can eliminate the visual "artifact" of seeing the "flipped"
          dinosaur underneath the floor by using stencil.  The idea is to
          draw the floor without color or depth update but so that
          a stencil value of one is where the floor will be.  Later when
          rendering the dinosaur reflection, we will only update pixels
          with a stencil value of 1 to make sure the reflection only
          lives on the floor, not below the floor. */

        /* Don't update color or depth. */
        glDisable(GL_DEPTH_TEST);
        glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

        /* Draw 1 into the stencil buffer. */
        glEnable(GL_STENCIL_TEST);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
        glStencilFunc(GL_ALWAYS, 1, 0xff);

        /* Now render floor; floor pixels just get their stencil set to 1. */
        drawFloor();

        /* Re-enable update of color and depth. */
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
        glEnable(GL_DEPTH_TEST);

        /* Now, only render where stencil is set to 1. */
        glStencilFunc(GL_EQUAL, 1, 0xff);  /* draw if ==1 */
        glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
      }

      glPushMatrix();

        /* The critical reflection step: Reflect dinosaur through the floor
           (the Y=0 plane) to make a relection. */
        glScalef(1.0, -1.0, 1.0);

       /* Reflect the light position. */
        glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

        glEnable(GL_NORMALIZE);
        glCullFace(GL_FRONT);

        /* Draw the reflected objects. */
        drawDinosaur();
        makeCube();
    
        /* Disable noramlize again and re-enable back face culling. */
        glDisable(GL_NORMALIZE);
        glCullFace(GL_BACK);

      glPopMatrix();

      /* Switch back to the unreflected light position. */
      glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);

      if (stencilReflection) {
        glDisable(GL_STENCIL_TEST);
      }
    }

    /* Draw "bottom" of floor in cyan. */
    glFrontFace(GL_CW);  /* Switch face orientation. */
    glColor4f(0.1, 0.7, 0.7, 1.0);
    drawFloor();
    glFrontFace(GL_CCW);

    if (renderShadow) {
      if (stencilShadow) {
       /* Draw the floor with stencil value 3.  This helps us only
          draw the shadow once per floor pixel (and only on the
          floor pixels). */
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(GL_ALWAYS, 3, 0xff);
        glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
      }
    }

    /* Draw "top" of floor.  Use blending to blend in reflection. */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(0.0, 0.0, 0.0, 0.3);
    glColor4f(1.0, 1.0, 1.0, 0.3);
    drawFloor();
    glDisable(GL_BLEND);

    if (renderDinosaur) {
      /* Draw "actual" objects, not its reflection. */
      drawDinosaur();
        makeCube();
    }

    if (renderShadow) {

      /* Render the projected shadow. */

      if (stencilShadow) {

        /* Now, only render where stencil is set above 2 (ie, 3 where
          the top floor is).  Update stencil with 2 where the shadow
          gets drawn so we don't redraw (and accidently reblend) the
          shadow). */
        glStencilFunc(GL_LESS, 2, 0xff);
        glStencilOp(GL_REPLACE, GL_REPLACE, GL_REPLACE);
      }

      /* To eliminate depth buffer artifacts, we use polygon offset
        to raise the depth of the projected shadow slightly so
        that it does not depth buffer alias with the floor. */
      if (offsetShadow) {
       switch (polygonOffsetVersion) {
       case EXTENSION:
#ifdef GL_EXT_polygon_offset
         glEnable(GL_POLYGON_OFFSET_EXT);
         break;
#endif
#ifdef GL_VERSION_1_1
       case ONE_DOT_ONE:
          glEnable(GL_POLYGON_OFFSET_FILL);
         break;
#endif
       case MISSING:
         /* Oh well. */
         break;
       }
      }

      /* Render 60% black shadow color on top of whatever the
         floor appareance is. */
      glEnable(GL_BLEND);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDisable(GL_LIGHTING);
      glColor4f(0.0, 0.0, 0.0, 0.6);

      glPushMatrix();
       /* Project the shadow. */
        glMultMatrixf((GLfloat *) floorShadow);
        drawDinosaur();
        makeCube();
      glPopMatrix();

      glDisable(GL_BLEND);
      glEnable(GL_LIGHTING);

      if (offsetShadow) {
       switch (polygonOffsetVersion) {
#ifdef GL_EXT_polygon_offset
       case EXTENSION:
         glDisable(GL_POLYGON_OFFSET_EXT);
         break;
#endif
#ifdef GL_VERSION_1_1
       case ONE_DOT_ONE:
          glDisable(GL_POLYGON_OFFSET_FILL);
         break;
#endif
       case MISSING:
         /* Oh well. */
         break;
       }
      }
      if (stencilShadow) {
        glDisable(GL_STENCIL_TEST);
      }
    }

    lights();

  glPopMatrix();
    glFinish();
  

  glutSwapBuffers();
}

static void mouse(int button, int state, int x, int y)
{
  if (button == GLUT_LEFT_BUTTON) {
    if (state == GLUT_DOWN) {
      moving = 1;
      startx = x;
      starty = y;
    }
    if (state == GLUT_UP) {
      moving = 0;
    }
  }
  if (button == GLUT_MIDDLE_BUTTON) {
    if (state == GLUT_DOWN) {
      lightMoving = 1;
      lightStartX = x;
      lightStartY = y;
    }
    if (state == GLUT_UP) {
      lightMoving = 0;
    }
  }
}

static void motion(int x, int y)
{
  if (moving) {
    angle = angle + (x - startx);
    angle2 = angle2 + (y - starty);
    startx = x;
    starty = y;
    glutPostRedisplay();
  }
  if (lightMoving) {
    lightAngle += (x - lightStartX)/40.0;
    lightHeight += (lightStartY - y)/20.0;
    lightStartX = x;
    lightStartY = y;
    glutPostRedisplay();
  }
}

static void idle()
{
  static float time = 0.0;

  time = glutGet(GLUT_ELAPSED_TIME) / 250.0;

  jump = 2.0 * fabs(sin(time)*0.5);
    
  if (!lightMoving) {
    lightAngle += 0.03;
  }
  glutPostRedisplay();
}



static void controlScene(int value)
{
  switch (value) {
  case F_NONE:
    return;
  case F_MOTION:
    animation = 1 - animation;
    if (animation) {
      glutIdleFunc(idle);
    } else {
      glutIdleFunc(NULL);
    }
    break;
  case F_LIGHT:
    lightSwitch = !lightSwitch;
    if (lightSwitch) {
      glEnable(GL_LIGHT0);
    } else {
      glDisable(GL_LIGHT0);
    }
    break;
  case F_TEXTURE:
    useTexture = !useTexture;
    break;
  case F_SHADOWS:
    renderShadow = 1 - renderShadow;
    break;
  case F_REFLECTION:
    renderReflection = 1 - renderReflection;
    break;
  case F_DINOSAUR:
    renderDinosaur = 1 - renderDinosaur;
    break;
  case F_STENCIL_REFLECTION:
    stencilReflection = 1 - stencilReflection;
    break;
  case F_STENCIL_SHADOW:
    stencilShadow = 1 - stencilShadow;
    break;
  case F_OFFSET_SHADOW:
    offsetShadow = 1 - offsetShadow;
    break;
  case F_POSITIONAL:
    directionalLight = 0;
    break;
  case F_DIRECTIONAL:
    directionalLight = 1;
    break;
  }
  glutPostRedisplay();
}

/* When not visible, stop animating.  Restart when visible again. */
static void visible(int vis)
{
  if (vis == GLUT_VISIBLE) {
    if (animation)
      glutIdleFunc(idle);
  } else {
    if (!animation)
      glutIdleFunc(NULL);
  }
}



/* Press any key to redraw; good when motion stopped and
   performance reporting on. */
static void key(unsigned char c, int x, int y)
{
  if (c == 27) {
    exit(0);  /* IRIS GLism, Escape quits. */
  }
  glutPostRedisplay();
}

/* Press any key to redraw; good when motion stopped and
   performance reporting on. */
static void special(int k, int x, int y)
{
  glutPostRedisplay();
}

static int supportsOneDotOne(void)
{
  const char *version;
  int major, minor;

  version = (char *) glGetString(GL_VERSION);
  if (sscanf(version, "%d.%d", &major, &minor) == 2)
    return major >= 1 && minor >= 1;
  return 0;
}

int
main(int argc, char **argv)
{
    
    v[0][0] = v[1][0] = v[2][0] = v[3][0] = 7;
    v[4][0] = v[5][0] = v[6][0] = v[7][0] = 5;
    v[0][1] = v[1][1] = v[4][1] = v[5][1] = 7;
    v[2][1] = v[3][1] = v[6][1] = v[7][1] = 5;
    v[0][2] = v[3][2] = v[4][2] = v[7][2] = 5;
    v[1][2] = v[2][2] = v[5][2] = v[6][2] = 7;
    
  

  glutInit(&argc, argv);

  for (int i=1; i<argc; i++) {
    if (!strcmp("-linear", argv[i])) {
      linearFiltering = 1;
    } else if (!strcmp("-mipmap", argv[i])) {
      useMipmaps = 1;
    } else if (!strcmp("-ext", argv[i])) {
      forceExtension = 1;
    }
  }

  glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL | GLUT_MULTISAMPLE);

  glutCreateWindow("Final Project");



  /* Register GLUT callbacks. */
  glutDisplayFunc(scene);
  glutMouseFunc(mouse);
  glutMotionFunc(motion);
  glutVisibilityFunc(visible);
  glutKeyboardFunc(key);
  glutSpecialFunc(special);
    
   // create Menu System for the program
    glutCreateMenu(controlScene);
    glutAddMenuEntry("Motion Tab", F_MOTION);
    glutAddMenuEntry("-----------------------", F_NONE);
    glutAddMenuEntry("Light Tab", F_LIGHT);
    glutAddMenuEntry("Texture Tab", F_TEXTURE);
    glutAddMenuEntry("Shadows Tab", F_SHADOWS);
    glutAddMenuEntry("Reflection Tab", F_REFLECTION);
    glutAddMenuEntry("Object(Dinosaur) Tab", F_DINOSAUR);
    glutAddMenuEntry("-----------------------", F_NONE);
    glutAddMenuEntry("Reflection Stenciling", F_STENCIL_REFLECTION);
    glutAddMenuEntry("Shadow Stenciling", F_STENCIL_SHADOW);
    glutAddMenuEntry("Shadow Offset", F_OFFSET_SHADOW);
    glutAddMenuEntry("----------------------", F_NONE);
    glutAddMenuEntry("Positional light", F_POSITIONAL);
    glutAddMenuEntry("Directional light", F_DIRECTIONAL);
    
  glutAttachMenu(GLUT_RIGHT_BUTTON);
  makeDinosaur();

#ifdef GL_VERSION_1_1
  if (supportsOneDotOne() && !forceExtension) {
    polygonOffsetVersion = ONE_DOT_ONE;
    glPolygonOffset(-2.0, -1.0);
  } else
#endif
  {
#ifdef GL_EXT_polygon_offset
  /* check for the polygon offset extension */
  if (glutExtensionSupported("GL_EXT_polygon_offset")) {
    polygonOffsetVersion = EXTENSION;
    glPolygonOffsetEXT(-0.1, -0.002);
  } else
#endif
    {
      polygonOffsetVersion = MISSING;

    }
  }

  glEnable(GL_CULL_FACE);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_TEXTURE_2D);
  glLineWidth(3.0);

  glMatrixMode(GL_PROJECTION);
  gluPerspective(40.0,1.0,20.0,100.0);
  glMatrixMode(GL_MODELVIEW);
  gluLookAt(0.0, 8.0, 60.0,0.0, 8.0, 0.0,0.0, 1.0, 0.0);

  glLightModeli(GL_LIGHT_MODEL_LOCAL_VIEWER, 1);
  glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
  glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 0.1);
  glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 0.05);
  glEnable(GL_LIGHT0);
  glEnable(GL_LIGHTING);

  makeFloorTexture();

  /* This function sets the floor plane for projected shadow calculations. */
  findPlane(floorPlane, floorVertices[1], floorVertices[2], floorVertices[3]);

  glutMainLoop();
  return 0;
}












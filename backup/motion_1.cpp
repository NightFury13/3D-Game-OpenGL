/* Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
/* File for "Putting It All Together" lesson of the OpenGL tutorial on
 * www.videotutorialsrock.com
 */


#include <stdio.h>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <math.h>
#include <set>
#include <sstream>
#include <stdlib.h>
#include <vector>

#ifdef __APPLE__
#include <OpenGL/OpenGL.h>
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif

#include "imageloader.h"
#include "md2model.h"
#include "text3d.h"

using namespace std;

//Global Variables.

//Bike.
float bike_x = 50.0f, bike_y, bike_z = 50.0f;
float bike_deltaAngle = 0.0f;
float bike_deltaMove = 0.0f;
float bike_angle = 0.0;
float bike_rot = 0.0;
float bike_lx = 0.0, bike_lz = 0.0;

//Camera.
float rot_fl = 0.0;

const float PI = 3.1415926535f;
//The width of the terrain in units, after scaling
const float TERRAIN_WIDTH = 50.0f;

//Returns a random float from 0 to < 1
float randomFloat() {
	return (float)rand() / ((float)RAND_MAX + 1);
}

//Represents a terrain, by storing a set of heights and normals at 2D locations
class Terrain {
	private:
		int w; //Width
		int l; //Length
		float** hs; //Heights
		Vec3f** normals;
		bool computedNormals; //Whether normals is up-to-date
	public:
		Terrain(int w2, int l2) {
			w = w2;
			l = l2;
			
			hs = new float*[l];
			for(int i = 0; i < l; i++) {
				hs[i] = new float[w];
			}
			
			normals = new Vec3f*[l];
			for(int i = 0; i < l; i++) {
				normals[i] = new Vec3f[w];
			}
			
			computedNormals = false;
		}
		
		~Terrain() {
			for(int i = 0; i < l; i++) {
				delete[] hs[i];
			}
			delete[] hs;
			
			for(int i = 0; i < l; i++) {
				delete[] normals[i];
			}
			delete[] normals;
		}
		
		int width() {
			return w;
		}
		
		int length() {
			return l;
		}
		
		//Sets the height at (x, z) to y
		void setHeight(int x, int z, float y) {
			hs[z][x] = y;
			computedNormals = false;
		}
		
		//Returns the height at (x, z)
		float getHeight(int x, int z) {
			return hs[z][x];
		}
		
		//Computes the normals, if they haven't been computed yet
		void computeNormals() {
			if (computedNormals) {
				return;
			}
			
			//Compute the rough version of the normals
			Vec3f** normals2 = new Vec3f*[l];
			for(int i = 0; i < l; i++) {
				normals2[i] = new Vec3f[w];
			}
			
			for(int z = 0; z < l; z++) {
				for(int x = 0; x < w; x++) {
					Vec3f sum(0.0f, 0.0f, 0.0f);
					
					Vec3f out;
					if (z > 0) {
						out = Vec3f(0.0f, hs[z - 1][x] - hs[z][x], -1.0f);
					}
					Vec3f in;
					if (z < l - 1) {
						in = Vec3f(0.0f, hs[z + 1][x] - hs[z][x], 1.0f);
					}
					Vec3f left;
					if (x > 0) {
						left = Vec3f(-1.0f, hs[z][x - 1] - hs[z][x], 0.0f);
					}
					Vec3f right;
					if (x < w - 1) {
						right = Vec3f(1.0f, hs[z][x + 1] - hs[z][x], 0.0f);
					}
					
					if (x > 0 && z > 0) {
						sum += out.cross(left).normalize();
					}
					if (x > 0 && z < l - 1) {
						sum += left.cross(in).normalize();
					}
					if (x < w - 1 && z < l - 1) {
						sum += in.cross(right).normalize();
					}
					if (x < w - 1 && z > 0) {
						sum += right.cross(out).normalize();
					}
					
					normals2[z][x] = sum;
				}
			}
			
			//Smooth out the normals
			const float FALLOUT_RATIO = 0.5f;
			for(int z = 0; z < l; z++) {
				for(int x = 0; x < w; x++) {
					Vec3f sum = normals2[z][x];
					
					if (x > 0) {
						sum += normals2[z][x - 1] * FALLOUT_RATIO;
					}
					if (x < w - 1) {
						sum += normals2[z][x + 1] * FALLOUT_RATIO;
					}
					if (z > 0) {
						sum += normals2[z - 1][x] * FALLOUT_RATIO;
					}
					if (z < l - 1) {
						sum += normals2[z + 1][x] * FALLOUT_RATIO;
					}
					
					if (sum.magnitude() == 0) {
						sum = Vec3f(0.0f, 1.0f, 0.0f);
					}
					normals[z][x] = sum;
				}
			}
			
			for(int i = 0; i < l; i++) {
				delete[] normals2[i];
			}
			delete[] normals2;
			
			computedNormals = true;
		}
		
		//Returns the normal at (x, z)
		Vec3f getNormal(int x, int z) {
			if (!computedNormals) {
				computeNormals();
			}
			return normals[z][x];
		}
};

//Loads a terrain from a heightmap.  The heights of the terrain range from
//-height / 2 to height / 2.
Terrain* loadTerrain(const char* filename, float height) {
	Image* image = loadBMP(filename);
	Terrain* t = new Terrain(image->width, image->height);
	for(int y = 0; y < image->height; y++) {
		for(int x = 0; x < image->width; x++) {
			unsigned char color =
				(unsigned char)image->pixels[3 * (y * image->width + x)];
			float h = height * ((color / 255.0f) - 0.5f);
			t->setHeight(x, y, h);
		}
	}
	
	delete image;
	t->computeNormals();
	return t;
}

//Returns the approximate height of the terrain at the specified (x, z) position
float heightAt(Terrain* terrain, float x, float z) {
	//Make (x, z) lie within the bounds of the terrain
	if (x < 0) {
		x = 0;
	}
	else if (x > terrain->width() - 1) {
		x = terrain->width() - 1;
	}
	if (z < 0) {
		z = 0;
	}
	else if (z > terrain->length() - 1) {
		z = terrain->length() - 1;
	}
	
	//Compute the grid cell in which (x, z) lies and how close we are to the
	//left and outward edges
	int leftX = (int)x;
	if (leftX == terrain->width() - 1) {
		leftX--;
	}
	float fracX = x - leftX;
	
	int outZ = (int)z;
	if (outZ == terrain->width() - 1) {
		outZ--;
	}
	float fracZ = z - outZ;
	
	//Compute the four heights for the grid cell
	float h11 = terrain->getHeight(leftX, outZ);
	float h12 = terrain->getHeight(leftX, outZ + 1);
	float h21 = terrain->getHeight(leftX + 1, outZ);
	float h22 = terrain->getHeight(leftX + 1, outZ + 1);
	
	//Take a weighted average of the four heights
	return (1 - fracX) * ((1 - fracZ) * h11 + fracZ * h12) +
		fracX * ((1 - fracZ) * h21 + fracZ * h22);
}


//Draws the terrain
void drawTerrain(Terrain* terrain) {
	glDisable(GL_TEXTURE_2D);
	glColor3f(0.3f, 0.9f, 0.0f);
	for(int z = 0; z < terrain->length() - 1; z++) {
		glBegin(GL_TRIANGLE_STRIP);
		for(int x = 0; x < terrain->width(); x++) {
			Vec3f normal = terrain->getNormal(x, z);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z), z);
			normal = terrain->getNormal(x, z + 1);
			glNormal3f(normal[0], normal[1], normal[2]);
			glVertex3f(x, terrain->getHeight(x, z + 1), z + 1);
		}
		glEnd();
	}
}

void drawBike() {

	glPushMatrix();
	glColor3f(0.1f, 0.1f, 0.1f);
	glTranslatef(0.75, 0.0, 1.0);
	glutSolidSphere(0.25, 5, 5); 
	glPopMatrix();
	glPushMatrix();
	glColor3f(0.1f, 0.1f, 0.1f);
	glTranslatef(-0.75, 0.0, 1.0);
	glutSolidSphere(0.25, 5, 5); 
	glPopMatrix();
	glPushMatrix();
	glColor3f(0.8f, 0.0f, 0.0f);
	glTranslatef(0.5, 0.0, 1.25);
	glutSolidCube(0.4);
	glPopMatrix();
	glPushMatrix();
	glColor3f(0.2f, 0.2f, 0.2f);
	glTranslatef(0.0, 0.0, 1.25);
	glutSolidCube(0.5);
	glPopMatrix();
	glPushMatrix();
	glColor3f(0.8f, 0.0f, 0.0f);
	glTranslatef(-0.5, 0.0, 1.25);
	glutSolidCube(0.5);
	glPopMatrix();
}


MD2Model* _model;
Terrain* _terrain;
float _angle = 0;

void cleanup() {
	delete _model;

	t3dCleanup();
}

void handleKeypress(unsigned char key, int x, int y) {
	switch (key) {
		case 27: //Escape key
			cleanup();
			exit(0);
	}
}


void pressSpecialKey(int key, int xx, int yy) 
{
	switch (key) {
		case GLUT_KEY_UP : bike_deltaMove = 1.0; break;
		case GLUT_KEY_DOWN : bike_deltaMove = -1.0; break;
		case GLUT_KEY_LEFT : bike_rot = 1.0; break;
		case GLUT_KEY_RIGHT : bike_rot = -1.0; break;
		case GLUT_KEY_F1 : rot_fl = 1.0; break;
		case GLUT_KEY_F2 : rot_fl = -1.0; break;
	}   
} 

void releaseSpecialKey(int key, int x, int y)  
{
	switch (key) {
		case GLUT_KEY_UP : bike_deltaMove = 0.0; break;
		case GLUT_KEY_DOWN : bike_deltaMove = 0.0; break;
		case GLUT_KEY_LEFT : bike_rot = 0.0; bike_angle += bike_deltaAngle; bike_deltaAngle = 0.0; break;
		case GLUT_KEY_RIGHT : bike_rot = 0.0; bike_angle += bike_deltaAngle; bike_deltaAngle = 0.0; break;
		case GLUT_KEY_F1 : rot_fl = 0.0; break;
		case GLUT_KEY_F2 : rot_fl = 0.0; break;
	}   
} 

//Makes the image into a texture, and returns the id of the texture
GLuint loadTexture(Image *image) {
	GLuint textureId;
	glGenTextures(1, &textureId);
	glBindTexture(GL_TEXTURE_2D, textureId);
	glTexImage2D(GL_TEXTURE_2D,
			0,
			GL_RGB,
			image->width, image->height,
			0,
			GL_RGB,
			GL_UNSIGNED_BYTE,
			image->pixels);
	return textureId;
}

void initRendering() {
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);
	glEnable(GL_NORMALIZE);
	glEnable(GL_COLOR_MATERIAL);
	glShadeModel(GL_SMOOTH);

	t3dInit(); //Initialize text drawing functionality

	//Load the model
	_model = MD2Model::load("blockybalboa.md2");
	if (_model != NULL) {
		_model->setAnimation("run");
	}
}

void handleResize(int w, int h) {
	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0, (float)w / (float)h, 1.0, 200.0);
}

void drawScene() {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();


	//The scaling factor for the terrain
	float scale = TERRAIN_WIDTH / (_terrain->width() - 1);

	glTranslatef(0, 0, -1.0f * scale * (_terrain->length() - 1));
	glRotatef(30, 1, 0, 0);
	glRotatef(_angle, 0, 1, 0);
	glTranslatef(-TERRAIN_WIDTH / 2, 0, -scale * (_terrain->length() - 1) / 2);

	GLfloat ambientLight[] = {0.5f, 0.5f, 0.5f, 1.0f};
	glLightModelfv(GL_LIGHT_MODEL_AMBIENT, ambientLight);

	GLfloat lightColor[] = {0.5f, 0.5f, 0.5f, 1.0f};
	GLfloat lightPos[] = {-0.2f, 0.3f, -1, 0.0f};
	glLightfv(GL_LIGHT0, GL_DIFFUSE, lightColor);
	glLightfv(GL_LIGHT0, GL_POSITION, lightPos);


	//Draw the terrain
	glScalef(scale, scale, scale);
	drawTerrain(_terrain);

	glColor3f(0.0, 0.0, 1.0);
	glTranslatef(bike_x, bike_y, bike_z);
	drawBike();

	glutSwapBuffers();
}

void update(int value) {

	if (rot_fl)
	{
		_angle += (rot_fl * 0.3f);
		printf("%f : rot_fl \n",rot_fl);
		if (_angle > 360.0) 
		{
			_angle -= 360.0;
		}
		if (_angle < 0.0) 
		{
			_angle += 360.0;
		}
	}

	if (bike_rot) {
		bike_deltaAngle = bike_deltaAngle + (bike_rot*0.05);
		
		bike_lx = cos(bike_angle + bike_deltaAngle);
		bike_lz = -sin(bike_angle + bike_deltaAngle);
	}

	if (bike_deltaMove) {
		bike_x += bike_deltaMove * bike_lx * 0.5;
		bike_z += bike_deltaMove * bike_lz * 0.5;
	}
		
	printf("x:%f y:%f z:%f rot:%f\n",bike_x, bike_y, bike_z, bike_angle);

	bike_y = _terrain->getHeight(bike_x, bike_z);

	glutPostRedisplay();
	glutTimerFunc(25, update, 0);
}

int main(int argc, char** argv) {
	srand((unsigned int)time(0)); //Seed the random number generator

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
	glutInitWindowSize(200, 200);

	glutCreateWindow("MotoCross Madness");
	initRendering();

	_terrain = loadTerrain("heightmap.bmp", 30.0f); //Load the terrain
	//Compute the scaling factor for the terrain
	//float scaledTerrainLength =
	//	TERRAIN_WIDTH / (_terrain->width() - 1) * (_terrain->length() - 1);

	glutDisplayFunc(drawScene);
	glutKeyboardFunc(handleKeypress);
	glutSpecialFunc(pressSpecialKey);
	glutSpecialUpFunc(releaseSpecialKey);
	glutReshapeFunc(handleResize);
	glutTimerFunc(25, update, 0);

	glutMainLoop();
	return 0;
}

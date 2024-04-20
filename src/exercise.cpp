
#include <cstdio>
#include <cstdlib>
#include <cmath> 
#include <ctime> 
#include <random> 
#include <vector>
#include <iostream>
#include <GL/glut.h>
#include <vmath>
#include <v3d>

void idle();
void display();
void reshape(int x, int y);
void render();

float lastTime      = 0.f;
float elapsedTime   = 0.f;
float deltaTime     = 0.f;

unsigned int slices = 7, stacks = 6;

int windowWidth   = 0;
int windowHeight  = 0;

/***********************************************************/

int main(int argc, char *argv[])
{
    glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
	glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
	glutCreateWindow("Exercise");
    
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
	glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glShadeModel(GL_SMOOTH);

    // for animation
    {
        lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
        // glutIdleFunc(idle);
    }
	glutMainLoop();

	return 0;
}

/////////////////////////////////////////////////

void renderPolygon(int sides, float radius)
{
	float const color1[] = { 1.f, 0.f, 0.f, 1.f };
	float const color2[] = { 0.f, 0.f, 1.f, 1.f };
	glBegin(GL_TRIANGLE_FAN);
	for(int i = 0; i < sides; i++)
	{
		float t = float(i)/sides;
		// glColor3f(
		// 	vm::lerp(t, color1[0], color2[0]),
		// 	vm::lerp(t, color1[1], color2[1]),
		// 	vm::lerp(t, color1[2], color2[2])
		// );
		t += 45.f/360.f;
		glVertex2f(radius * std::sin(vm::norm2rad(t)), radius * std::cos(vm::norm2rad(t)));
	}
	glEnd();
}

void render()
{
	using namespace vm;
    glPushMatrix();
		glColor3f(1.f,1.f,1.f);
		renderPolygon(4, .3f);
		//
		float k = .5f; // x = x+k*y
		mat4 mat1 = mat4(
			1, k, 0, 0,
			k, 1, 0, 0,
			0, 0, 1, 0,
			0, 0, 0, 1
		); 
		glPushMatrix();
			glMultMatrixf(transpose(translate(.5f) * mat1).ptr());
			glColor3f(1.f,0.f,0.f);
			renderPolygon(4, .3f);
		glPopMatrix();
		// //
		// mat4 mat1 = rotate_z(deg2rad(45.f)) * translate(.5f, 0.f); 
		// glPushMatrix();
		// 	glMultMatrixf(transpose(mat1).ptr());
		// 	glColor3f(1.f,0.f,0.f);
		// 	renderPolygon(4, .1f);
		// glPopMatrix();
		// //
		// mat4 mat2 = translate(.5f, 0.f) * rotate_z(deg2rad(45.f));
		// glPushMatrix();
		// 	glMultMatrixf(transpose(mat2).ptr());
		// 	glColor3f(0.f,0.f,1.f);
		// 	renderPolygon(4, .1f);
		// glPopMatrix();
    glPopMatrix();
}

void idle()
{
	glutPostRedisplay();
}

void display()
{
    float currentTime = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
    deltaTime = currentTime - lastTime;
    lastTime = currentTime;
	elapsedTime += deltaTime;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.f, 0.f, 5.f);
	render();
	glutSwapBuffers();
}

void reshape(int x, int y)
{
    float const znear = 0.1f;
	float const aspect = (float)x / (float)y;
	windowWidth = x;
	windowHeight = y;
    // vm::mat4 mProjection = vm::ortho<float>(-aspect, aspect, -1, 1, -500.f, 500.0f);
    // vm::mat4 mProjection = vm::fov<float>(50.f, aspect, znear, 500.0);
	glViewport(0, 0, x, y);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(-1, 1, -1, 1, -1, 1);
	// glTranslatef(0.f,0.f,.5f);

    // glLoadMatrixf(vm::transpose(mProjection).ptr());
	glMatrixMode(GL_MODELVIEW);
}

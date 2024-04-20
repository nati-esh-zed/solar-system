
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

static char const *helpPrompt[] = {"Press F1 for help", 0};
static char const *helpText[] = {
	"Rotate: left mouse drag",
	" Scale: right mouse drag up/down",
	"   Pan: middle mouse drag",
	"",
	"Toggle fullScreen: f",
	"Toggle switchLabels: l",
	"Toggle switchTrails: t",
	"Toggle switchRotate: r",
	"Toggle animation: space",
	"Quit: escape",
	0
};

void idle();
void display();
void reshape(int x, int y);
void keyPress(unsigned char key, int x, int y);
void sKeyPress(int key, int x, int y);
void mouse(int bn, int st, int x, int y);
void motion(int x, int y);
void printHelp();
void printFps();

void renderSun();
void renderMercury();
void renderVenus();
void renderEarth();
void renderMars();
void renderJupiter();
void renderSaturn();
void renderUranus();
void renderNeptune();
void renderSolarSystem(); 
void renderText(char const * text, float position[2]);

float lastTime      = 0.f;
float elapsedTime   = 0.f;
float deltaTime     = 0.f;
float lastFrameTime = 0.f;
float fps     = 0.f;
long  nFrames = 0;

int windowWidth   = 0;
int windowHeight  = 0;
float camTheta  = 0.f;
float camPhi    = 25;
float camDist   = 8;
float camPan[3] = {};
float camScale  = 1.f/5.f;
int mouseX = 0;
int mouseY = 0;
bool buttonState[8]  = {};
bool switchAnimation = true;
bool switchLabels    = true;
bool switchTrails    = true;
bool switchRotate    = true;
bool switchHelp      = false;

float orbitDurationMercury = 88.f;
float orbitDurationVenus = 225.f;
float orbitDurationEarth = 365.f;
float orbitDurationEarthMoon = 27.f;
float orbitDurationMars  = 687.f;
float orbitDurationJupiter = 4333.f;
float orbitDurationSaturn = 10759.f;
float orbitDurationUranus = 30687.f;
float orbitDurationNeptune = 60190.f;

float orbitOffsetMercury = 0.0f;
float orbitOffsetVenus = 0.5f;
float orbitOffsetEarth = 0.7f;
float orbitOffsetEarthMoon = 0.f;
float orbitOffsetMars  = 0.3f;
float orbitOffsetJupiter = 0.4f;
float orbitOffsetSaturn = 0.1f;
float orbitOffsetUranus = 0.6f;
float orbitOffsetNeptune = 0.2f;

/***********************************************************/

float orbitDurationPerSec = orbitDurationEarth / 20.f;
float starDistance = 200.f;
int   numStars     = 1000;
std::vector<vm::vec3> starGens;

/***********************************************************/

int main(int argc, char *argv[])
{
    // generate random star coordinates to project onto sphere.  
    { 
        starGens.reserve(numStars);
        std::random_device rd; // obtain a random number from hardware
        std::mt19937 gen(rd()); // seed the generator
        std::uniform_real_distribution<float> distR(.2f, .3f);
        std::uniform_real_distribution<float> distZ(0.f, 360.f);
        std::uniform_real_distribution<float> distY(0.f, 360.f);
        for(int i = 0; i < numStars; i++)
        {
            float radius = distR(gen);
            float y = distY(gen);
            float z = distZ(gen);
            starGens.push_back({radius, y, z});
        }
    }

    glutInit(&argc, argv);
	glutInitWindowSize(800, 600);
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_MULTISAMPLE);
	glutCreateWindow("Solar System");
    
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyPress);
	glutSpecialFunc(sKeyPress);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);

    glEnable(GL_LINE_SMOOTH);
    glEnable(GL_POLYGON_SMOOTH);
	glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
	glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
	glEnable(GL_LIGHTING);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHT0);
    glShadeModel(GL_SMOOTH);

    // initialize light0 parameters
    {
        float r = 1.f;
        float g = .95f;
        float b = .8f;
        float ambientFactor = .05f;
        float specularFactor = .7f;
        float lightAmbient[] = {r*ambientFactor, g*ambientFactor,b*ambientFactor, 1.f};
        float lightDiffuse[] = {r, g, b, 1.f};
        float lightSpecular[] = {r*specularFactor, g*specularFactor,b*specularFactor, 1.f};
        glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmbient);
        glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiffuse);
        glLightfv(GL_LIGHT0, GL_SPECULAR, lightSpecular);
        glLightf(GL_LIGHT0, GL_CONSTANT_ATTENUATION, 4.0f);
        glLightf(GL_LIGHT0, GL_LINEAR_ATTENUATION, 1.0f);
        glLightf(GL_LIGHT0, GL_QUADRATIC_ATTENUATION, 0.25f);
    }

    // for animation
    {
        lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
        glutIdleFunc(switchAnimation ? idle : 0);
    }

	glutMainLoop();
	return 0;
}

/////////////////////////////////////////////////

struct Material {
    vm::vec4 diffuse;
    vm::vec4 emission = {0.f, 0.f, 0.f, 1.f};
    vm::vec4 ambient  = diffuse;
    vm::vec4 specular = {diffuse.x, diffuse.y, diffuse.z, .5f};
    // vm::vec4 specular = {1.f, 1.f, 1.f, .3f};
};

void renderPlanet(char const * label, float radius, float distance
                , Material material = {}, float tiltAngle = 0.f
                , float orbitDuration = 0.f, float orbitOffset = 0.f
                , void (*child)() = nullptr)
{
    vm::mat4 mPlanet = vm::identity<float>();
    vm::mat4 mTilt   = vm::rotate_z(vm::deg2rad(tiltAngle));
    vm::mat4 mPlace  = vm::translate(distance, 0.f, 0.f);
    vm::mat4 mScale  = vm::scale(radius, radius, radius);
    if(orbitDuration != 0.f) 
    {
        float orbitAngle = 360.f*(orbitOffset+elapsedTime*orbitDurationPerSec/orbitDuration);
        vm::mat4 mOrbit = vm::rotate_y(vm::deg2rad(orbitAngle));
        mPlanet = mTilt * mOrbit * mPlace;
        // render trail
        if(switchTrails)
        {
            glPushAttrib(GL_ENABLE_BIT);
            glDisable(GL_LIGHTING);
            glPushMatrix();
            glBegin(GL_LINE_STRIP);
            for(int i = 0; i <= 10; i++) 
            {
                float inorm = float(i) / 10.f;
                float lineColor[] = { 1.f, 1.f, 1.f, 1.f - inorm };
                glColor4fv(lineColor);
                float lineDist = 50;
                float theta = inorm * lineDist;
                vm::mat4 mPath = vm::rotate_y(-vm::deg2rad(theta));
                vm::vec3 v = (mTilt * (mPath * (mOrbit * (mPlace * vm::vec3{}))));
                glVertex3f(v.x, v.y, v.z);
            }
            glEnd();
            glPopMatrix();
	        glPopAttrib();
        }
    }
    else
        mPlanet = mTilt * mPlace;
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE|GL_EMISSION);
    glMaterialfv(GL_FRONT, GL_AMBIENT, material.ambient.ptr());
    glMaterialfv(GL_FRONT, GL_DIFFUSE, material.diffuse.ptr());
    glMaterialfv(GL_FRONT, GL_SPECULAR, material.specular.ptr());
    glMaterialfv(GL_FRONT, GL_EMISSION, material.emission.ptr());
    glMaterialf(GL_FRONT, GL_SHININESS, material.specular.w);
    static v3d::SolidSphere sphere(1.f, 28, 24);
    glColor4fv(material.diffuse.ptr());
    glPushMatrix();
    glMultMatrixf(transpose(mPlanet).ptr());
    glPushMatrix();
    glMultMatrixf(transpose(mScale).ptr());
    sphere.render();
    glPopMatrix();
    if(switchLabels && label != nullptr)
    {
        float textPosition[] = {radius, radius};
        renderText(label, textPosition);
    }
    if(child != nullptr)
        child();
    glPopMatrix();
}

void renderRingedPlanet(char const * label, float radius, float distance
                      , float ringRadius = 0.f, float ringSize = 0.f
                      , Material material = {}, Material ringMaterial = {}
                      , float tiltAngle = 0.f, float orbitDuration = 0.f, float orbitOffset = 0.f)
{
    vm::mat4 mPlanet = vm::identity<float>();
    vm::mat4 mTilt   = vm::rotate_z(vm::deg2rad(tiltAngle));
    vm::mat4 mPlace  = vm::translate(distance, 0.f, 0.f);
    vm::mat4 mScale  = vm::scale(radius, radius, radius);
    if(orbitDuration != 0.f) 
    {
        float orbitAngle = 360.f*(orbitOffset+elapsedTime*orbitDurationPerSec/orbitDuration);
        vm::mat4 mOrbit = vm::rotate_y(vm::deg2rad(orbitAngle));
        mPlanet = mTilt * mOrbit * mPlace * mScale;
        // render trail
        if(switchTrails)
        {
            glPushAttrib(GL_ENABLE_BIT);
            glDisable(GL_LIGHTING);
            glPushMatrix();
            glBegin(GL_LINE_STRIP);
            for(int i = 0; i <= 10; i++) 
            {
                float inorm = float(i) / 10.f;
                float lineColor[] = { 1.f, 1.f, 1.f, 1.f - inorm };
                glColor4fv(lineColor);
                float lineDist = 50;
                float theta = inorm * lineDist;
                vm::mat4 mPath = vm::rotate_y(-vm::deg2rad(theta));
                vm::vec3 v = (mTilt * (mPath * (mOrbit * (mPlace * vm::vec3{}))));
                glVertex3f(v.x, v.y, v.z);
            }
            glEnd();
            glPopMatrix();
	        glPopAttrib();
        }
    }
    else
        mPlanet = mTilt * mPlace * mScale;
    static v3d::SolidSphere sphere(1.f, 28, 24);
    glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE|GL_EMISSION);
    glMaterialfv(GL_FRONT, GL_AMBIENT, material.ambient.ptr());
    glMaterialfv(GL_FRONT, GL_DIFFUSE, material.diffuse.ptr());
    glMaterialfv(GL_FRONT, GL_SPECULAR, material.specular.ptr());
    glMaterialfv(GL_FRONT, GL_EMISSION, material.emission.ptr());
    glMaterialf(GL_FRONT, GL_SHININESS, material.specular.w);
    glColor4fv(material.diffuse.ptr());
    glPushMatrix();
    glMultMatrixf(transpose(mPlanet).ptr());
    glutSolidSphere(1.f, 28, 24);
    sphere.render();
    if(ringSize > 0.f)
    {
        glColorMaterial(GL_FRONT, GL_AMBIENT_AND_DIFFUSE|GL_EMISSION);
        glMaterialfv(GL_FRONT, GL_AMBIENT, ringMaterial.ambient.ptr());
        glMaterialfv(GL_FRONT, GL_DIFFUSE, ringMaterial.diffuse.ptr());
        glMaterialfv(GL_FRONT, GL_SPECULAR, ringMaterial.specular.ptr());
        glMaterialfv(GL_FRONT, GL_EMISSION, ringMaterial.emission.ptr());
        glMaterialf(GL_FRONT, GL_SHININESS, ringMaterial.specular.w);
        glColor4fv(ringMaterial.diffuse.ptr());
        // glutSolidTorus(ringSize/radius, ringRadius/radius, 2, 45);
        static v3d::SolidTorus torus(ringSize/radius, ringRadius/radius, 45, 2);
        torus.render();
    }
    if(switchLabels && label != nullptr)
    {
        float textPosition[] = {radius, radius};
        renderText(label, textPosition);
    }
    glPopMatrix();
}

void renderStar(float radius, float yzProjections[2], float color[4]) {
    vm::mat4 mStar = 
          vm::rotate_y<float>(yzProjections[0]) 
        * vm::rotate_z<float>(yzProjections[1]) 
        * vm::translate<float>(starDistance, 0, 0)
        * vm::scale(radius, radius, radius);
    glColor4fv(color);
    glColorMaterial(GL_FRONT, GL_EMISSION);
    glMaterialfv(GL_FRONT, GL_EMISSION, color);
    glPushMatrix();
    glMultMatrixf(transpose(mStar).ptr());
    static v3d::SolidSphere sphere(1.f, 4, 4);
    sphere.render();
    glPopMatrix();
}

void renderBackground() 
{
    for(size_t i = 0; i < starGens.size(); i++) 
    {
        vm::vec3 const & starGen = starGens[i];
        float color[4] = {1.f,1.f,1.f,1.f};
        float yzProjections[2] = { starGen.y, starGen.z };
        renderStar(starGen.x, yzProjections, color);
    }
}

void renderSun() 
{
    static Material material = {{1.f, 0.6f, 0.3f, 1.f}, {1.f, 0.6f, 0.3f, 1.f}};
    static Material haloMaterial = {{1.f, 0.77f, 0.6f, .1f}, {1.f, 0.77f, 0.6f, 1.f}};
    renderPlanet("Sun", .8f, 0.f, material, 0, 0);
    float r1 = vm::map<float>(std::sin(vm::norm2rad(1.f/2.f)*elapsedTime+0.0f), -1, 1, 1.1f, 1.3f);
    float r2 = vm::map<float>(std::sin(vm::norm2rad(1.f/2.f)*elapsedTime+0.03f), -1, 1, 1.3f, 1.5f);
    float r3 = vm::map<float>(std::sin(vm::norm2rad(1.f/2.f)*elapsedTime+0.07f), -1, 1, 1.4f, 1.7f);
    renderPlanet(nullptr, r1, 0.f, haloMaterial, 0, 0);
    renderPlanet(nullptr, r2, 0.f, haloMaterial, 0, 0);
    renderPlanet(nullptr, r3, 0.f, haloMaterial, 0, 0);
}

void renderMercury() 
{
    static Material material = { {0.48f, 0.25f, 0.09f, 1.f} };
    renderPlanet("Mercury", .34f, 3.f, material, -10, orbitDurationMercury, orbitOffsetMercury);
}

void renderVenus() 
{
    static Material material = { {.84f, 0.67f, 0.55f, 1.f} };
    renderPlanet("Venus", .4f, 5.f, material, 30, orbitDurationVenus, orbitOffsetVenus);
}

void renderEarth() 
{
    static Material material = { {0.19f, 0.78f, 0.95f, 1.f} };
    renderPlanet("Earth", .45f, 7.f, material, 0, orbitDurationEarth, orbitOffsetEarth,
    []() 
    {
        glPushMatrix();
        glRotatef(90.f, 1.f, 0.f, 0.f);
        static Material moonMaterial = { {0.9f, 0.9f, 0.9f, 1.f} };
        renderPlanet("Moon", .1f, .8f, moonMaterial, 0, orbitDurationEarthMoon, orbitOffsetEarthMoon);
        glPopMatrix();
    });
}

void renderMars() 
{
    static Material material = { {.83f, 0.24f, 0.16f, 1.f} };
    renderPlanet("Mars", .4f, 9.f, material, 20, orbitDurationMars, orbitOffsetMars);
}

void renderJupiter() 
{
    static Material material = { {.6f, 0.26f, 0.12f, 1.f} };
    renderPlanet("Jupiter", .8f, 11.f, material, -20, orbitDurationJupiter, orbitOffsetJupiter);
}

void renderSaturn()  
{
    static Material material = { {.96f, 0.95f, 0.70f, 1.f} };
    static Material ringMaterial = { {.97f, 0.88f, 0.81f, .5f} };
    renderRingedPlanet("Saturn", .6f, 14.f, .9f, .14f, material, ringMaterial, 
        15, orbitDurationSaturn, orbitOffsetSaturn);
}

void renderUranus() 
{
    static Material material = { {.46f, 0.82f, 0.70f, 1.f} };
    static Material ringMaterial = { {.97f, 0.88f, 0.81f, .5f} };
    renderRingedPlanet("Uranus", .4f, 17.f, .7f, .05f, material, ringMaterial, 
        20, orbitDurationUranus, orbitOffsetUranus);
}

void renderNeptune()  
{
    static Material material = { {.0f, 0.65f, 0.88f, 1.f} };
    renderPlanet("Neptune", .4f, 19.f, material, 0, orbitDurationNeptune, orbitOffsetNeptune);
}

void renderSolarSystem() 
{
    glPushMatrix();
    renderMercury();
    renderVenus();
    renderEarth();
    renderMars();
    renderJupiter();
    renderSaturn();
    renderUranus();
    renderNeptune();
    renderSun();
    glPopMatrix();
}

void renderText(char const * text, float position[2])
{
	char const * ch;

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	glPushMatrix();
    glColor3f(0.1f, 0.1f, 0.1f);
    glRasterPos2f(position[0]+.01f, position[1]-.01f);
    ch = &text[0];
    while(*ch) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *ch++);
    }
    glColor3f(1.f, 0.f, 0.4f);
    glRasterPos2f(position[0], position[1]);
    ch = &text[0];
    while(*ch) {
        glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *ch++);
    }
	glPopMatrix();
	glPopAttrib();
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
    if(switchAnimation)
        elapsedTime += deltaTime;
    if(switchAnimation && switchRotate)
        camTheta += 3.f * deltaTime;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
    vm::mat4 mCameraScale  = vm::scale(camScale, camScale, camScale);
    vm::mat4 mCameraPan    = vm::translate(camPan[0], camPan[1], camPan[2]);
    vm::mat4 mCameraOrbitY = vm::rotate_y(vm::deg2rad(camTheta));
    vm::mat4 mCameraTiltX  = vm::rotate_x(vm::deg2rad(camPhi));
    vm::mat4 mCameraZoom  = vm::translate(0.f, 0.f, -camDist);
    vm::mat4 mCamera = mCameraZoom * mCameraTiltX * mCameraOrbitY * mCameraPan * mCameraScale;
    glLoadMatrixf(vm::transpose(mCamera).ptr());

	float lightPosition[] = {0.f, 0.f, 0.f, 1.f};
    glLightfv(GL_LIGHT0, GL_POSITION, lightPosition);
    
    glPushMatrix();
        renderBackground();
        renderSolarSystem();
        printHelp();
        printFps();
    glPopMatrix();

	glutSwapBuffers();
    // calculate FPS
    {
        nFrames++;
        float frameTime = currentTime - lastFrameTime;
        if(frameTime > 1.f) {
            fps = float(nFrames) / frameTime;
            nFrames = 0;
            lastFrameTime = currentTime;
        }
    }
}

void reshape(int x, int y)
{
    float const znear = 0.1f;
	float const aspect = (float)x / (float)y;
	windowWidth = x;
	windowHeight = y;
    // vm::mat4 mProjection = vm::ortho<float>(-aspect, aspect, -1, 1, znear, 500.0);
    vm::mat4 mProjection = vm::fov<float>(50.f, aspect, znear, 500.0);
	glViewport(0, 0, x, y);
	glMatrixMode(GL_PROJECTION);
    // gluPerspective(50, aspect, znear, 500);
    glLoadMatrixf(vm::transpose(mProjection).ptr());
	glMatrixMode(GL_MODELVIEW);
}

void mouse(int bn, int st, int x, int y)
{
	int bidx = bn - GLUT_LEFT_BUTTON;
	buttonState[bidx] = st == GLUT_DOWN;
	mouseX = x;
	mouseY = y;
}

void motion(int x, int y)
{
	int dx = x - mouseX;
	int dy = y - mouseY;
	mouseX = x;
	mouseY = y;

	if(!(dx | dy)) return;

	if(buttonState[0]) {
		camTheta += dx * 0.5f;
		camPhi += dy * 0.5f;
		if(camPhi < -90.f) 
            camPhi = -90.f;
		if(camPhi > 90.f) 
            camPhi = 90.f;
		glutPostRedisplay();
	}
	if(buttonState[1]) {
		float up[3], right[3];
		float theta = camTheta * vm::PI / 180.0f;
		float phi = camPhi * vm::PI / 180.0f;

		up[0] = -sin(theta) * sin(phi);
		up[1] = -cos(phi);
		up[2] = cos(theta) * sin(phi);
		right[0] = cos(theta);
		right[1] = 0;
		right[2] = sin(theta);

		camPan[0] += (right[0] * dx + up[0] * dy) * 0.01f;
		camPan[1] += up[1] * dy * 0.01f;
		camPan[2] += (right[2] * dx + up[2] * dy) * 0.01f;
		glutPostRedisplay();
	}
	if(buttonState[2]) {
		camDist += dy * 0.1f;
		if(camDist < 0) camDist = 0;
		glutPostRedisplay();
	}
}

void keyPress(unsigned char key, int x, int y)
{
	static int fullScreen;
	static int prev_xsz, prev_ysz;

	switch(key) {
	case 27:
	case 'q':
		exit(0);
		break;
	case 'l':
		switchLabels ^= 1;
		break;
	case 't':
		switchTrails ^= 1;
		break;
	case 'r':
		switchRotate ^= 1;
		break;
	case ' ':
		switchAnimation ^= 1;
		glutIdleFunc(switchAnimation ? idle : 0);
		glutPostRedisplay();
		if(switchAnimation)
        {
            lastTime = glutGet(GLUT_ELAPSED_TIME) / 1000.f;
            lastFrameTime = lastTime;
            nFrames = 0;
        }
		break;
	case '\n':
	case '\r':
		if(!(glutGetModifiers() & GLUT_ACTIVE_ALT)) {
			break;
		}
	case 'f':
		fullScreen ^= 1;
		if(fullScreen) {
			prev_xsz = glutGet(GLUT_WINDOW_WIDTH);
			prev_ysz = glutGet(GLUT_WINDOW_HEIGHT);
			glutFullScreen();
		} else {
			glutReshapeWindow(prev_xsz, prev_ysz);
		}
		break;
	}
}

void sKeyPress(int key, int x, int y)
{
	switch(key) {
	case GLUT_KEY_F1:
		switchHelp ^= 1;
		glutPostRedisplay();

	default:
		break;
	}
}

void printHelp()
{
	int i;
	char const *s, **text;

	glPushAttrib(GL_ENABLE_BIT);
	glDisable(GL_LIGHTING);
	glDisable(GL_DEPTH_TEST);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glMatrixMode(GL_PROJECTION);
	glPushMatrix();
	glLoadIdentity();
	glOrtho(0, windowWidth, 0, windowHeight, -1, 1);

	text = switchHelp ? helpText : helpPrompt;

	for(i=0; text[i]; i++) {
		glColor3f(0.f, 0.1f, 0.f);
		glRasterPos2f(7.f, float(windowHeight - (i + 1) * 20 - 2));
		s = text[i];
		while(*s) {
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
		}
		glColor3f(0.f, 0.9f, 0.f);
		glRasterPos2f(5.f, float(windowHeight - (i + 1) * 20));
		s = text[i];
		while(*s) {
			glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
		}
	}

	glPopMatrix();
	glMatrixMode(GL_MODELVIEW);

	glPopAttrib();
}
 
void printFps()
{
    if(switchAnimation) {
	    char const *s, *text;
        char buffer[128] = {};
        snprintf(buffer, sizeof(buffer), "%4.0f FPS", fps);
        text = buffer;
        glPushAttrib(GL_ENABLE_BIT);
        glDisable(GL_LIGHTING);
        glDisable(GL_DEPTH_TEST);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glOrtho(0, windowWidth, 0, windowHeight, -1, 1);
        glColor3f(0.f, 0.1f, 0.f);
        glRasterPos2f(7.f, 20 );
        s = text;
        while(*s) {
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
        }
        glColor3f(0.9f, 0.9f, 0.f);
        glRasterPos2f(5.f, 20+2);
        s = text;
        while(*s) {
            glutBitmapCharacter(GLUT_BITMAP_9_BY_15, *s++);
        }
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopAttrib();
    }

}

#include <iostream>
using namespace::std;

#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>

#if defined(__APPLE__)
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#include <GLUT/glut.h>
#else
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__)
#include <windows.h>
#endif
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>
#endif


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Innentol modosithatod...

//--------------------------------------------------------
// 3D Vektor
//--------------------------------------------------------
struct Vector {
   float x, y, z;

   Vector( ) {
	x = y = z = 0;
   }
   Vector(float x0, float y0, float z0 = 0) {
	x = x0; y = y0; z = z0;
   }
   Vector operator*(float a) {
	return Vector(x * a, y * a, z * a);
   }
   Vector operator+(const Vector& v) {
 	return Vector(x + v.x, y + v.y, z + v.z);
   }
   Vector operator-(const Vector& v) {
 	return Vector(x - v.x, y - v.y, z - v.z);
   }
   float operator*(const Vector& v) { 	// dot product
	return (x * v.x + y * v.y + z * v.z);
   }
   Vector operator%(const Vector& v) { 	// cross product
	return Vector(y*v.z-z*v.y, z*v.x - x*v.z, x*v.y - y*v.x);
   }
   float Length() { return sqrt(x * x + y * y + z * z); }
   Vector normalize() {
        float l = Length();
        return Vector(x/l, y/l, z/l);
   }

   Vector operator/(float a) {
       float one_per = 1.0/a;
       return Vector(x * one_per, y * one_per, z * one_per);
   }


};


struct Point : Vector {
    float parameter;
    Vector speed;
    Vector center;

    Point() {};
    Point(Vector v, float param) {
        this->x = v.x;
        this->y = v.y;
        parameter = param;
        speed = Vector(0, 0);
        center = Vector(v.x, v.y - 5.0);
    }
};


//--------------------------------------------------------
// Spektrum illetve szin
//--------------------------------------------------------
struct Color {
   float r, g, b;

   Color( ) {
	r = g = b = 0;
   }
   Color(float r0, float g0, float b0) {
	r = r0; g = g0; b = b0;
   }
   Color operator*(float a) {
	return Color(r * a, g * a, b * a);
   }
   Color operator*(const Color& c) {
	return Color(r * c.r, g * c.g, b * c.b);
   }
   Color operator+(const Color& c) {
 	return Color(r + c.r, g + c.g, b + c.b);
   }
};

const int screenWidth = 600;	// alkalmazás ablak felbontása
const int screenHeight = 600;

#define SPHERE_NTESS 50
#define BODY_NTESS 80
#define CYLINDER_NTESS 20
#define QUAD_NTESS 10

unsigned int planet_texture_id;
unsigned int space_texture_id;

void generate_textures();


class CR {
public:
    int size;
    Point points[10];

    CR() {
        size = 0;
    }

    void add_point(float x, float y, float z, float parameter) {
        Point p = Point(Vector(x, y, z), parameter);
        if(size > 1) {
            points[size - 1].speed = (
                    ((points[size]-points[size-1])/(points[size].parameter-points[size-1].parameter))
                    +
                    ((points[size-1]-points[size-2])/(points[size-1].parameter-points[size-2].parameter))
            ) * 0.5;
        }
        points[size++] = p;
    }

    void draw_points() {
        glColor3f(0, 0, 0);
        for(int i=0; i<size; i++) {
            // draw circle
        }
    }

    void draw_cr() {
        glColor3f(0, 1, 0);
        glBegin(GL_LINE_STRIP);
        for(int i = 0; i < size - 1; i++) {
            for(int j = 0; j <= 100; j++) {
                Vector p = get_cr(i, i+1, j/100.0);
                glVertex3f(p.x, p.y, p.z);
            }
        }
        glEnd();
    }

    Vector get_u(float n, bool derivate=false) {
        float max_weight = points[size-1].parameter;
        float weight = max_weight * n;
        int i = 0;
        while(weight > points[i].parameter)
            i++;
        if(weight < 0.1) i=1;
        float part = (weight-points[i-1].parameter)/(points[i].parameter-points[i-1].parameter);
        return get_cr(i-1, i, part, derivate);
        //cout << weight << " " << i-1 << endl;
    }

    Vector get_cr(int n0, int n1, float part, bool derivate=false) {
        Point p0 = points[n0];
        Point p1 = points[n1];

        part = (p1.parameter - p0.parameter) * part;
        Vector v0 = p0;
        Vector v1 = p0.speed;
        Vector v2 = (p1 -p0)*3 / pow(p1.parameter-p0.parameter,2) - (p1.speed + p0.speed *2)
                    /
                    (p1.parameter-p0.parameter);
        Vector v3 = (p1.speed + p0.speed) / pow(p1.parameter - p0.parameter,2) - (p1 -p0)*2
                    /
                    pow(p1.parameter - p0.parameter,3);
        return !derivate ? v0 + v1*part + v2*(part*part) + v3 *(part*part*part) :
                                v1      + v2 * 2.0 * part + v3 * 3.0 * (part * part);
    }

} cr;

class  CRBody {
public:

    CRBody() {}

    Vector get_uv_vector(float u, float v) {
        float b = v * 3.14 * 2.0;
        Vector curve_point = cr.get_u(u);

        Vector p =
                    Vector(curve_point.z * sin(b) + curve_point.x * cos(b),
                           curve_point.y,
                           curve_point.z * cos(b) - curve_point.x * sin(b));
        return p;
    }

    void draw() {
        glBegin(GL_QUADS);
        for(int u=0; u<BODY_NTESS; u++) {
            for(int v=0; v<=BODY_NTESS; v++) {
                Vector a = get_uv_vector((float)u/BODY_NTESS,       (float)v/BODY_NTESS);
                Vector b = get_uv_vector((float)(u+1)/BODY_NTESS,   (float)v/BODY_NTESS);
                Vector c = get_uv_vector((float)(u+1)/BODY_NTESS,   (float)(v+1)/BODY_NTESS);
                Vector d = get_uv_vector((float)u/BODY_NTESS,       (float)(v+1)/BODY_NTESS);

                Vector normal = ((d-a) % (b - a)).normalize();
                glNormal3f(normal.x, normal.y, normal.z);
                glVertex3f(a.x, a.y, a.z);

                normal = ((a -b) % (c - b)).normalize();
                glNormal3f(normal.x, normal.y, normal.z);
                glVertex3f(b.x, b.y, b.z);

                normal = ((b - c) % (d - c)).normalize();
                glNormal3f(normal.x, normal.y, normal.z);
                glVertex3f(c.x, c.y, c.z);

                normal = ((c - d) % (a - d)).normalize();
                glNormal3f(normal.x, normal.y, normal.z);
                glVertex3f(d.x, d.y, d.z);
            }
        }
        glEnd();
    }
};

void VertexSphere(float u, float v, bool in) {
        float l = u * 2.0 * 3.14;
        float b = v * 3.14;

        Vector p = Vector(
                sin(b) * cos(l),
                sin(b) * sin(l),
                cos(b)
        );
        Vector normal = (p - Vector(0, 0, 0)).normalize() * (in ? -1 : 1);
        glTexCoord2f(u, v);
        glNormal3f(normal.x, normal.y, normal.z);
        glVertex3f(p.x, p.y, p.z);
}

void glSphere(bool in=false) {
    glBegin(GL_QUADS);
    for(int v=0; v<SPHERE_NTESS; v++) {
        for(int u=0; u<SPHERE_NTESS; u++) {
            VertexSphere((float)u/SPHERE_NTESS,       (float)v/SPHERE_NTESS, in);
            VertexSphere((float)(u+1)/SPHERE_NTESS,   (float)v/SPHERE_NTESS, in);
            VertexSphere((float)(u+1)/SPHERE_NTESS,   (float)(v+1)/SPHERE_NTESS, in);
            VertexSphere((float)u/SPHERE_NTESS,       (float)(v+1)/SPHERE_NTESS, in);
        }
    }
    glEnd();
}

void glQuad(Vector a, Vector b, Vector c, Vector d) {
    Vector normal = ((b - a) % (d - a)).normalize();
    glBegin(GL_QUADS);
    for(int u=0; u<QUAD_NTESS; u++) {
        for(int v=0; v<QUAD_NTESS; v++) {
            Vector p = (a + (b-a)*u/QUAD_NTESS) + ((d-a)*v/QUAD_NTESS);
            glNormal3f(normal.x, normal.y, normal.z);
            glVertex3f(p.x, p.y, p.z);
            p = (a + (b-a)*(u+1)/QUAD_NTESS) + ((d-a)*v/QUAD_NTESS);
            glNormal3f(normal.x, normal.y, normal.z);
            glVertex3f(p.x, p.y, p.z);
            p = (a + (b-a)*(u+1)/QUAD_NTESS) + ((d-a)*(v+1)/QUAD_NTESS);
            glNormal3f(normal.x, normal.y, normal.z);
            glVertex3f(p.x, p.y, p.z);
            p = (a + (b-a)*u/QUAD_NTESS) + ((d-a)*(v+1)/QUAD_NTESS);
            glNormal3f(normal.x, normal.y, normal.z);
            glVertex3f(p.x, p.y, p.z);
        }
    }
    glEnd();

}

void glCube() {
        Vector A(0.5, -0.5, -0.5), B(-0.5, -0.5, -0.5), C(-0.5, 0.5, -0.5), D(0.5, 0.5, -0.5),
               E(0.5, -0.5,  0.5), F(-0.5, -0.5,  0.5), G(-0.5, 0.5,  0.5), H(0.5, 0.5,  0.5);

        glQuad(A, B, C, D); glQuad(H, G, F, E);
        glQuad(D, C, G, H); glQuad(E, F, B, A);
        glQuad(E, A, D, H); glQuad(C, B, F, G);
}

void VertexCylinder(float u, float v) {
    u = u * 3.14 * 2.0;
    v = v * 1;
    Vector p = Vector(
            1.0 * cos(u),
            v,
            1.0 * sin(u)
    );

    glTexCoord2f(u, v);
    Vector normal(p.x, 0, p.z);
    glNormal3f(normal.x, normal.y, normal.z);
    glVertex3f(p.x, p.y, p.z);
}

void glFakeCylinder() {
    glBegin(GL_QUADS);
    for(int v=0; v<=CYLINDER_NTESS; v++) {
        for(int u=0; u<=CYLINDER_NTESS; u++) {
            VertexCylinder((float)u/CYLINDER_NTESS,       (float)v/CYLINDER_NTESS);
            VertexCylinder((float)(u+1)/CYLINDER_NTESS,   (float)v/CYLINDER_NTESS);
            VertexCylinder((float)(u+1)/CYLINDER_NTESS,   (float)(v+1)/CYLINDER_NTESS);
            VertexCylinder((float)u/CYLINDER_NTESS,       (float)(v+1)/CYLINDER_NTESS);
        }
    }
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0, 0, 0);
    for(int i=0; i<=CYLINDER_NTESS; i+=1) {
        Vector p = Vector(
                        cos(2 * 3.14 * i/CYLINDER_NTESS),
                        0,
                        sin(2 * 3.14 * i/CYLINDER_NTESS)
                );
        glNormal3f(0, -1, 0);
        glVertex3f(p.x, p.y, p.z);

    }
    glEnd();
}


CRBody body;

float specular[]={1, 1, 1, 1};
float ambient[]={0.25f, 0.25f, 0.25f};
float white[]={1, 1, 1, 1};
float red[] = {1, 0, 0, 1};
float black[] = {0, 0, 0, 1};
float green[] = {0, 1, 0, 1};
float blue[] = {0, 0, 1, 1};
float light_blue[] = {21/255.0, 178/255.0, 255/255.0, 0.2};
float dark_blue[] = {0/255.0, 0/255.0, 60/255.0, 1};
float yellowish[]={1, 1, 194/255.0, 1};
float copper_d[] = {0.7038, 0.27048, 0.0828, 1};
float copper_s[] = {0.256777, 0.137622, 0.086014};


void glSpaceship() {
    glMaterialfv( GL_FRONT, GL_DIFFUSE, dark_blue);
    float dis[] = {0,0,0,1};
    glMaterialfv( GL_FRONT, GL_SPECULAR, dis);

    glPushMatrix();
        glPushMatrix();
        glTranslatef(0, 2.2, 0);
        glRotatef(-45, 0, 1, 0);
        glScalef(12, 0.1, 2);
        glCube();
        glPopMatrix(); glPushMatrix();
        glTranslatef(0, 2.2, 0);
        glRotatef(45, 0, 1, 0);
        glScalef(12, 0.1, 2);
        glCube();
    glPopMatrix(); glPushMatrix();
        glMaterialfv( GL_FRONT, GL_DIFFUSE, black);
        glMaterialfv( GL_FRONT, GL_AMBIENT, dis);
        glTranslatef(0, 3.25, -1.5);
        glRotatef(-28, 1, 0, 0);
        glScalef(0.8, 0.1, 0.8);
        glSphere();
        glMaterialfv( GL_FRONT, GL_AMBIENT, ambient);
    glPopMatrix();

    glMaterialfv( GL_FRONT, GL_DIFFUSE, copper_d);
    glMaterialfv( GL_FRONT, GL_SPECULAR, copper_s);
    glMaterialf( GL_FRONT, GL_SHININESS, 10);
    body.draw();
    glPopMatrix();
}

void glPlanet() {
    float dis[]={0, 0, 0, 0};
    glMaterialfv( GL_FRONT, GL_SPECULAR, dis);
    glPushMatrix();
        glPushMatrix();
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glBindTexture(GL_TEXTURE_2D, planet_texture_id);
        glSphere();
        glDisable(GL_TEXTURE_2D);

        glPopMatrix(); glPushMatrix();
        glMaterialfv( GL_FRONT, GL_DIFFUSE, light_blue);
        glScalef(1.1, 1.1, 1.1);
        glSphere();
        glPopMatrix();

    glPopMatrix();
}

void glSpace() {
    glPushMatrix();
        glEnable(GL_TEXTURE_2D);
        glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
        glBindTexture(GL_TEXTURE_2D, space_texture_id);
        glMaterialfv( GL_FRONT, GL_DIFFUSE, yellowish);

        glRotatef(45, 1, 0, 0);
        glScalef(80, 80, 80);
        glSphere(true);
        glDisable(GL_TEXTURE_2D);
    glPopMatrix();
}

void glSat() {
    glPushMatrix();
        glRotatef(45, 1, 1, 0);
        glPushMatrix();
            glMaterialfv( GL_FRONT, GL_DIFFUSE, red);
            glTranslatef(1.5, 0, 0);
            glRotatef(90, 0, 0, 1);
            glScalef(0.2, 1, 0.2);
            glFakeCylinder();
        glPopMatrix(); glPushMatrix();
            glMaterialfv( GL_FRONT, GL_DIFFUSE, red);
            glTranslatef(-1.5, 0, 0);
            glRotatef(-90, 0, 0, 1);
            glScalef(0.2, 1, 0.2);
            glFakeCylinder();
        glPopMatrix(); glPushMatrix();
            glMaterialfv( GL_FRONT, GL_DIFFUSE, blue);
            glTranslatef(0, -1.5, 0);
            //glRotatef(90, 0, 1, 0);
            glScalef(0.2, 1, 0.2);
            glFakeCylinder();
        glPopMatrix(); glPushMatrix();
            glMaterialfv( GL_FRONT, GL_DIFFUSE, blue);
            glTranslatef(0, 1.5, 0);
            glRotatef(180, 1, 0, 0);
            glScalef(0.2, 1, 0.2);
            glFakeCylinder();
        glPopMatrix(); glPushMatrix();
            glMaterialfv( GL_FRONT, GL_DIFFUSE, green);
            glTranslatef(0, 0, -1.5);
            glRotatef(90, 1, 0, 0);
            glScalef(0.2, 1, 0.2);
            glFakeCylinder();
        glPopMatrix(); glPushMatrix();
            glMaterialfv( GL_FRONT, GL_DIFFUSE, green);
            glTranslatef(0, 0, 1.5);
            glRotatef(-90, 1, 0, 0);
            glScalef(0.2, 1, 0.2);
            glFakeCylinder();
        glPopMatrix();

        glMaterialfv( GL_FRONT, GL_DIFFUSE, white);
        glMaterialfv( GL_FRONT, GL_SPECULAR, specular);
        glMaterialf( GL_FRONT, GL_SHININESS, 5);
        glSphere();
    glPopMatrix();
}

Color image[screenWidth*screenHeight];	// egy alkalmazás ablaknyi kép

// Inicializacio, a program futasanak kezdeten, az OpenGL kontextus letrehozasa utan hivodik meg (ld. main() fv.)
void onInitialization( ) {
    glViewport(0, 0, screenWidth, screenHeight);
    glEnable(GL_DEPTH_TEST);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(60, 1, 0.1, 100);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0, 2, -5,  0, 2, 0,  0, 1, 0);

    glCullFace(GL_FRONT_AND_BACK);
    glEnable(GL_NORMALIZE);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);

    float p[4] = {0, 20.0f, -55.0f, 1};
    glLightfv(GL_LIGHT0, GL_POSITION, p);
    float c[4] = {0.7f, 0.8f, 0.9f, 1.0f};
    glLightfv(GL_LIGHT0, GL_DIFFUSE, c);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glMaterialfv(GL_FRONT, GL_AMBIENT, ambient);

    // texture
    glGenTextures(1, &planet_texture_id);
    glBindTexture(GL_TEXTURE_2D, planet_texture_id);


    generate_textures();

    cr.add_point(0,   0, 0, 0);
    cr.add_point(1.5, 1, 0, 10);
    cr.add_point(2.5, 2, 0, 15);
    cr.add_point(0.5, 4, 0, 30);
    cr.add_point(0,   5, 0, 35);

}

float rot_spaceship = -45.0;
float rot_planet = 0;
float rot_sat = 45.0;

// Rajzolas, ha az alkalmazas ablak ervenytelenne valik, akkor ez a fuggveny hivodik meg
void onDisplay( ) {
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);		// torlesi szin beallitasa
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // kepernyo torles

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    glPushMatrix();
        glSpace();
    glPopMatrix(); glPushMatrix();
        glTranslatef(-2, 1, 0);
        glRotatef(rot_sat, 1, 0, 0);
        glScalef(0.5, 0.5, 0.5);
        glSat();
    glPopMatrix(); glPushMatrix();
        glTranslatef(-2, 3, 8);
        glRotatef(rot_spaceship, 1, 0, 0);
        glScalef(0.4, 0.4, 0.4);
        glSpaceship();
    glPopMatrix(); glPushMatrix();
        glTranslatef(5, 0, 5);
        glRotatef(rot_planet, 0, 1, 0);
        glScalef(3, 3, 3);
        glPlanet();
    glPopMatrix();

    glBegin(GL_LINES);
        glVertex3f(-2, 3, 8);
        glVertex3f(0, -2, 0);
    glEnd();

    glutSwapBuffers();

}

// Billentyuzet esemenyeket lekezelo fuggveny (lenyomas)
void onKeyboard(unsigned char key, int x, int y) {
    if (key == 'd') glutPostRedisplay( ); 		// d beture rajzold ujra a kepet

}

// Billentyuzet esemenyeket lekezelo fuggveny (felengedes)
void onKeyboardUp(unsigned char key, int x, int y) {

}

// Eger esemenyeket lekezelo fuggveny
void onMouse(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)   // A GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON illetve GLUT_DOWN / GLUT_UP
		glutPostRedisplay( ); 						 // Ilyenkor rajzold ujra a kepet
}

// Eger mozgast lekezelo fuggveny
void onMouseMotion(int x, int y) {

}

// `Idle' esemenykezelo, jelzi, hogy az ido telik, az Idle esemenyek frekvenciajara csak a 0 a garantalt minimalis ertek
void onIdle( ) {
     long time = glutGet(GLUT_ELAPSED_TIME);		// program inditasa ota eltelt ido
     /*
     rot_spaceship = time / 100.0;
     rot_planet = time / 200.0;
     rot_sat = time / 20.0;
     */

     glutPostRedisplay();

}

void generate_textures() {
    float texture[256*256*3];
    for(int i=0; i<256*256;i++) {
        texture[i*3]=0;
        texture[i*3+1]=(rand()%256)/128.0;
        texture[i*3+2]=0;
    }

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 256, 256, 0, GL_RGB, GL_FLOAT, texture);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_MODULATE);

    float color;
    float texture2[512*512*3];
    for(int i=0; i<512*512;i++) {
        color = rand()%1000 > 997 ? 1.0 : 0.0;
        texture2[i*3]=color;
        texture2[i*3+1]=color;
        texture2[i*3+2]=color;
    }

    glGenTextures(2, &space_texture_id);
    glBindTexture(GL_TEXTURE_2D, space_texture_id);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 512, 512, 0, GL_RGB, GL_FLOAT, texture2);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexEnvi(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE, GL_MODULATE);
}


// ...Idaig modosithatod
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

// A C++ program belepesi pontja, a main fuggvenyt mar nem szabad bantani
int main(int argc, char **argv) {
    glutInit(&argc, argv); 				// GLUT inicializalasa
    glutInitWindowSize(600, 600);			// Alkalmazas ablak kezdeti merete 600x600 pixel
    glutInitWindowPosition(100, 100);			// Az elozo alkalmazas ablakhoz kepest hol tunik fel
    glutInitDisplayMode(GLUT_RGBA | GLUT_DOUBLE | GLUT_DEPTH);	// 8 bites R,G,B,A + dupla buffer + melyseg buffer

    glutCreateWindow("Grafika hazi feladat");		// Alkalmazas ablak megszuletik es megjelenik a kepernyon

    glMatrixMode(GL_MODELVIEW);				// A MODELVIEW transzformaciot egysegmatrixra inicializaljuk
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);			// A PROJECTION transzformaciot egysegmatrixra inicializaljuk
    glLoadIdentity();

    onInitialization();					// Az altalad irt inicializalast lefuttatjuk

    glutDisplayFunc(onDisplay);				// Esemenykezelok regisztralasa
    glutMouseFunc(onMouse);
    glutIdleFunc(onIdle);
    glutKeyboardFunc(onKeyboard);
    glutKeyboardUpFunc(onKeyboardUp);
    glutMotionFunc(onMouseMotion);

    glutMainLoop();					// Esemenykezelo hurok

    return 0;
}


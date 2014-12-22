#define _USE_MATH_DEFINES
#include <math.h>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>

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

using namespace::std;

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
   Vector operator/(float a) {
    return Vector(x / a, y / a, z / a);
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


void draw_circle(Vector center, float r, Color color) {
    glColor3f(color.r, color.g, color.b);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(center.x, center.y);
    for(int j=0; j <= 101; j++) {
        float x = r * sin(2 * 3.14 * j/100.0);
        float y = r * cos(2 * 3.14 * j/100.0);
        glVertex2f(center.x + x, center.y + y);
    }
    glEnd();
}


struct Camera {
public:
    float x, y, xsize, ysize;
    Camera(float x=0, float y=0, float xsize=58, float ysize=68):x(x),y(y),xsize(xsize),ysize(ysize) {}
} camera;

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


class Manager {
public:
    int size;
    Point points[10];
    float first_parameter;
    long orbit_start_time;
    int selected;

    Manager() {
        size = 0;
        orbit_start_time = -1;
        selected = -1;
    }

    void add_point(float x, float y, long time) {
        float parameter = time / 1000.0;
        if(size > 0) {
            parameter -= first_parameter;
        }
        else {
            first_parameter = parameter;
            parameter = 0;
        }

        Point p = Point(Vector(x, y), parameter);
        if(size > 1) {
            points[size - 1].speed = (
                    ((points[size]-points[size-1])/(points[size].parameter-points[size-1].parameter))
                    +
                    ((points[size-1]-points[size-2])/(points[size-1].parameter-points[size-2].parameter))
            ) * 0.5;
        }

        points[size++] = p;
    }

    bool is_selected() {
        return selected != -1;
    }

    void right_click(float x, float y) {
        for(int i=0; i<size; i++) {
            float distance = sqrt(pow(x - points[i].x, 2) + pow(y - points[i].y, 2));
            if(distance < 2.0) {
                selected = i;
            }
        }
    }

    Vector camera_bottom_left() {
        if(is_selected()) {
            return Vector(points[selected].x - camera.xsize / 2.0,
                          points[selected].y - camera.ysize / 2.0);
        } else {
            return Vector(0, 0);
        }
    }

    bool is_orbiting() {
        return orbit_start_time != -1;
    }

    void start_orbit(long time) {
        orbit_start_time = time;
    }

    void update_orbit_positions(long time) {
        float deg = ((time - orbit_start_time) % 5000) / 5000.0;
        for(int i=0; i<size; i++) {
            deg = 1 - deg;

            float x = 5 * sin(2 * 3.14 * deg);
            float y = 5 * cos(2 * 3.14 * deg);
            points[i].x = points[i].center.x + x;
            points[i].y = points[i].center.y + y;
        }
    }

    void draw() {
        if(size > 1) {
            draw_convex();
            draw_cr();
            draw_bzr();
            draw_cc();
        }
        draw_points();
    }

    void draw_points() {
        glColor3f(0, 0, 0);
        for(int i=0; i<size; i++) {
            draw_circle(points[i], 2.0, Color(0, 0, 0));
        }
    }

    float calc_deg(Vector p1, Vector p2) {
        float rad;
        if(p1.x == p2.x)
            rad = 3.141592/2.0;
        else
            rad = atan((p2.y - p1.y)/(p2.x - p1.x));
        return (rad < 0 ? 3.14 + rad  : rad) * 180.0/3.141592;
    }

    bool is_left(Vector r1, Vector r2, Vector p) {
        return ((r2-r1)%(p-r1)) * Vector(0, 0, 1) > 0;
    }

    void draw_convex() {
        Vector convex[10];
        for(int i=0; i<size; i++)
            convex[i] = Vector(points[i].x, points[i].y);

        int min_y = 0;
        for(int i=1; i<size; i++) {
            if(convex[min_y].y > convex[i].y)
                min_y = i;
        }

        Vector tmp = convex[min_y];
        convex[min_y] = convex[0];
        convex[0] = tmp;

        for(int i=1; i<size;i++)
            for(int j=1; j<size; j++)
                if(calc_deg(convex[0], convex[i]) < calc_deg(convex[0], convex[j])) {
                    tmp = convex[i];
                    convex[i] = convex[j];
                    convex[j] = tmp;
                }

        int convex_points[10];
        convex_points[0] = 0;
        convex_points[1] = 1;
        int cp = 2;
        for(int i=2; i<size; i++) {
            int can_be = 1;
            for(int j=i+1; j<size; j++) {
                if(i==j) continue;
                if(!is_left(convex[convex_points[cp-1]], convex[i], convex[j])) {
                    can_be = 0;
                }
            }
            if(can_be)
                convex_points[cp++] = i;
        }

        glColor3f(95/225.0, 243/255.0, 245/255.0);
        glBegin(GL_TRIANGLE_FAN);
        for(int i=0; i<cp; i++) {
            glVertex2f(convex[convex_points[i]].x, convex[convex_points[i]].y);
        }
        glEnd();
    }

    void draw_cc() {
        Vector greens[500];
        Vector old_points[500];
        Vector new_points[500];

        for(int i=0; i<size; i++)
            old_points[i] = Vector(points[i].x, points[i].y);

        int op=size;
        int g=0;
        int np=0;
        for(int c=0; c<4; c++) {
            g = 0;
            for(int i=0; i<op-1; i++) {
                Vector v = (Vector(old_points[i].x, old_points[i].y) +
                            Vector(old_points[i+1].x, old_points[i+1].y))/2.0;
                greens[g++] = v;
            }

            np = 0;
            new_points[np++] = Vector(old_points[0].x, old_points[0].y);
            int i;
            for(i=1; i<op-1; i++) {
                new_points[np++] = greens[i-1];
                Vector prev_next = (greens[i-1] + greens[i])/4.0;
                new_points[np++] = old_points[i]/2.0 + prev_next;
            }
            new_points[np++] = greens[i-1];
            new_points[np++] = Vector(old_points[op-1].x, old_points[op-1].y);


            for(int i=0; i<np; i++) {
                old_points[i] = new_points[i];
            }
            op=np;
        }
        glBegin(GL_LINE_STRIP);
        glColor3f(0, 0, 1);
        for(int i=0; i<np; i++) {
            glVertex2f(new_points[i].x, new_points[i].y);
        }
        glEnd();
    }

    void draw_cr() {
        glColor3f(0, 1, 0);
        glBegin(GL_LINE_STRIP);
        for(int i = 0; i < size - 1; i++) {
            for(int j = 0; j <= 100; j++) {
                Vector p = get_cr(i, i+1, j/100.0);
                glVertex2f(p.x, p.y);
            }
        }
        glEnd();
    }


    Vector get_cr(int n0, int n1, float part) {
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
        return v0 + v1*part + v2*(part*part) + v3 *(part*part*part);
    }


    void draw_bzr() {
        glColor3f(1, 0, 0);
        glBegin(GL_LINE_STRIP);
        for(float i = 0; i<=1; i+=1/100.0) {
            Vector t = r(i);
            glVertex2f(t.x, t.y);
        }
        glEnd();

    }

    float B(int i, float t) {
        float choose = 1;
        for(int j = 1; j <= i; j++)
            choose *= (float)(size-1-j+1)/j;
        return choose * pow(t, i) * pow(1-t, size-1-i);
    }

    Vector r(float t) {
        Vector rr(0, 0);
        for(int i = 0; i < size; i++)
            rr = rr + points[i] * B(i, t);
        return rr;
    }

} manager;


const int screenWidth = 600;	// alkalmazás ablak felbontása
const int screenHeight = 600;


Color image[screenWidth*screenHeight];	// egy alkalmazás ablaknyi kép


// Inicializacio, a program futasanak kezdeten, az OpenGL kontextus letrehozasa utan hivodik meg (ld. main() fv.)
void onInitialization( ) {
    glViewport(0, 0, screenWidth, screenHeight);

    gluOrtho2D(camera.x, camera.x + camera.xsize, camera.y, camera.y + camera.ysize);

}

// Rajzolas, ha az alkalmazas ablak ervenytelenne valik, akkor ez a fuggveny hivodik meg
void onDisplay( ) {
    glClearColor(244/255.0, 244/255.0, 244/255.0, 1.0f);		// torlesi szin beallitasa
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // kepernyo torles

    Vector v = manager.camera_bottom_left();
    camera.x = v.x;
    camera.y = v.y;
    glLoadIdentity();
    gluOrtho2D(camera.x, camera.x + camera.xsize, camera.y, camera.y + camera.ysize);


    manager.draw();

    glutSwapBuffers();     				// Buffercsere: rajzolas vege

}

// Billentyuzet esemenyeket lekezelo fuggveny (lenyomas)
void onKeyboard(unsigned char key, int x, int y) {
    long time = glutGet(GLUT_ELAPSED_TIME);		// program inditasa ota eltelt ido
    if (key == 32)
        manager.start_orbit(time);

    glutPostRedisplay( ); 		// d beture rajzold ujra a kepet

}

// Billentyuzet esemenyeket lekezelo fuggveny (felengedes)
void onKeyboardUp(unsigned char key, int x, int y) {

}

// Eger esemenyeket lekezelo fuggveny
void onMouse(int button, int state, int x, int y) {
    y = screenHeight - y;

    float x1 = camera.x + (x / 600.0 * camera.xsize);
    float y1 = camera.y + (y / 600.0 * camera.ysize);
    long time = glutGet(GLUT_ELAPSED_TIME);

    // A GLUT_LEFT_BUTTON / GLUT_RIGHT_BUTTON illetve GLUT_DOWN / GLUT_UP
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        manager.add_point(x1, y1, time);
    }

    if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) {
        manager.right_click(x1, y1);
    }
    glutPostRedisplay( );
}

// Eger mozgast lekezelo fuggveny
void onMouseMotion(int x, int y)
{

}

// `Idle' esemenykezelo, jelzi, hogy az ido telik, az Idle esemenyek frekvenciajara csak a 0 a garantalt minimalis ertek
void onIdle( ) {
    long time = glutGet(GLUT_ELAPSED_TIME);		// program inditasa ota eltelt ido
    if(manager.is_orbiting())
        manager.update_orbit_positions(time);
    glutPostRedisplay( );
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


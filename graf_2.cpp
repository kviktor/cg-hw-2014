#include <iostream>

using namespace std;

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
   float length() { return sqrt(x * x + y * y + z * z); }
   Vector normalize() {
       float l = length();
       return Vector(x/l, y/l, z/l);
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


class Versor {
    Versor(float  w, Vector v)
        : w(w), v(v)
    {}

public:
    float w;
    Vector v;

    Versor() {}
    Versor(Vector axis, float angle)
        : w(cos(angle * 0.5f)), v(axis.normalize() * sin(angle * 0.5f))
    {
    }

    Versor inverse() {
        return Versor(w, v * -1);
    }

    Versor operator*(Versor rhs) {
        return Versor(w * rhs.w - (v * rhs.v), rhs.v * w + v * rhs.w + (v % rhs.v));
    }

    Vector rotate(Vector vector) {
        Vector  t = (v % vector) * 2.0;
        return vector + t * w + (v % t);
    }
};

#define FLOAT_MAX       (2 << 16)
#define EPSILON         0.002

const int screenWidth = 600;	// alkalmazás ablak felbontása
const int screenHeight = 600;

Color image[screenWidth*screenHeight];	// egy alkalmazás ablaknyi kép


class Ray {
public:
    Vector src;
    Vector dir;

    Ray(Vector src, Vector dir):src(src) {
        this->dir = dir.normalize();
    }

    Vector along(float distance) {
        return src + dir * distance;
    }

    Ray translate(Vector  & translation) {
        return Ray(src + translation, dir);
    }

    Ray rotate(Versor  & rotation) {
        return Ray(rotation.rotate(src), rotation.rotate(dir));
    }
};

class Camera {
public:
    Vector eye;
    Vector look_at;
    Vector up;
    Vector right;

    Camera(Vector eye, Vector look_at, Vector up):eye(eye),look_at(look_at){
        this->up = up.normalize();
        right = ((look_at - eye).normalize() % this->up).normalize();
        this->up = right.normalize() % (look_at - eye).normalize();
    }

    Ray get_ray(int x, int y) {
        Vector src = look_at +
                     right * (2.0 * (x + 0.5)/600 - 1) +
                     up    * (2.0 * (y + 0.5)/600 - 1);
        Vector dir = (src - eye).normalize();
        return Ray(src, dir);
    }

} camera(
    //Vector(-5, 0, 4), Vector(0, 0, 4),
       Vector(-4, 0, 6),
        Vector(2, 0, 5),
        Vector(0, 0, 1));


class Light {
public:
    Vector point;
    float power;
    Color color;

    Light() {}
    Light(Vector p, Color c, float power) {
        this->power = power;
        this->point = p;
        this->color = c;
    }
};

class Material {
public:
    Color n, k;
    bool is_diffuse;
    bool is_textured;
    bool is_reflective;
    bool is_refractive;
    Color color;
    Material(bool t):is_diffuse(true),is_textured(true) {}
    Material(Color n, Color k, bool is_reflective, bool is_refractive):n(n),k(k),
        is_diffuse(false),is_reflective(is_reflective),is_refractive(is_refractive) {}

    Color get_color(Vector v = Vector(0,0,0)) {
        if(!is_textured) return color;

        float x = fabs(fmod(v.x+1000, 2.0));
        float y = fabs(fmod(v.y+1000, 2.0));
        if( x > 1) {
            if(y > 1) {
                return Color(1, 1, 1);
            }
            return Color(0, 0, 0);
        } else {
            if(y > 1)
                return Color(0, 0, 0);
        }
        return Color(1, 1, 1);
    }

    float fresnel(float n, float k, float cos_theta) {
        float F0 = ((n-1)*(n-1) + (k*k)) / ((n+1)*(n+1) + (k*k));
        return F0 + (1 - F0) * pow(1-cos_theta, 5);
    }

    Color fresnel(float cos_theta) {
        return Color(fresnel(n.r, k.r, cos_theta), fresnel(n.g, k.g, cos_theta), fresnel(n.b, k.b, cos_theta));
    }
}
 gold(Color(0.17f, 0.35f, 1.5f), Color(3.1f, 2.7f, 1.9f), true, false),
 silver(Color(0.14f, 0.16f, 0.13), Color(4.1f, 2.3f, 3.1f), true, false),
 glass(Color(1.5f, 1.5f, 1.5), Color(0.0f, 0.0f, 0.0f), true, true),
 textured(true);


class Object {
public:
    Material* material;
    Object() {}
    Object(Material* mat):material(mat) {}
    virtual bool intersect(Ray ray, float* distance, Vector* normal) = 0;
};

class Triange : public Object {
public:
    Vector r1, r2, r3, normal;

    Triange(Vector a, Vector b, Vector c, Material* mat):Object(mat) {
        r1 = a;
        r2 = b;
        r3 = c;
        normal = ((r2-r1).normalize() % (r3 - r1).normalize()).normalize();
    }

    bool intersect(Ray ray, float* distance, Vector* normal) {
        if (ray.dir * this->normal == 0)
            return false;

        *distance = ((r1 - ray.src) * this->normal)
                    /
                    (ray.dir * this->normal);


        if(*distance < EPSILON) return false;

        Vector p = ray.src + ray.dir * (*distance);
        if(
                ((r2 - r1) % (p - r1)) * this->normal > 0 &&
                ((r3 - r2) % (p - r2)) * this->normal > 0 &&
                ((r1 - r3) % (p - r3)) * this->normal > 0
          ) {
            *normal = this->normal;
            return true;
        }
        return false;
    }
} table1(Vector(15, 6, 0), Vector(15, -6, 0), Vector(50, -6, 0), &textured),
  table2(Vector(15, 6, 0), Vector(50, -6, 0), Vector(50, 6, 0), &textured);



class Ellipsoid  : public Object
{
public:
    Vector center;
    float radius;
    float radius_z;
    Versor rot;

    Ellipsoid(Vector o, float r, float r_z, Material* mat, Versor v):Object(mat),rot(v) {
        center = o;
        radius = r;
        radius_z = r_z;
    }

    Ray to_local(Ray ray) {
        Ray r(ray.src-center, ray.dir); // translate

        r = r.rotate(rot); // rotate

        r.src.z = r.src.z / radius_z; // scale
        r.src.x = r.src.x / radius;
        r.src.y = r.src.y / radius;

        r.dir.z = r.dir.z / radius_z;
        r.dir.x = r.dir.x / radius;
        r.dir.y = r.dir.y / radius;
        r.dir = r.dir.normalize();
        return r;
    }

    Vector inverse_transform(Vector v) {
        return rot.inverse().rotate(Vector(v.x * radius, v.y * radius, v.z * radius_z)) + center;
    }

    Vector inverse_transform_normal(Vector normal) {
        return rot.inverse().rotate(Vector(normal.x / radius, normal.y / radius, normal.z / radius_z)).normalize();
    }

    float inverse_transform(Ray r, float distance) {
        Vector i = r.along(distance);
        i = inverse_transform(i);
        Vector src = inverse_transform(r.src);
        return (i-src).length();
    }

    bool intersect(Ray ray, float* distance, Vector* normal) {
        ray = to_local(ray);
        float dx = ray.dir.x;
        float dy = ray.dir.y;
        float dz = ray.dir.z;
        float x0 = ray.src.x;
        float y0 = ray.src.y;
        float z0 = ray.src.z;

        float a = dx * dx + dy * dy + dz * dz;
        float b = 2.0 * dx * x0 + 2.0 * dy * y0 + 2.0 * dz * z0;
        float c = x0 * x0 + y0 * y0 + z0 * z0 - 1.0;
        float d = b * b - 4.0 * a * c;
        if(d < 0) {
            return false;
        }

        float t = ((-1.0 * b - sqrt(d)) / (2.0 * a));
        if(t > EPSILON) {
            *distance = inverse_transform(ray, t);
            *normal = inverse_transform_normal(((ray.src + ray.dir * t))).normalize();
            return true;
        }

        return false;
    }
};


class Paraboloid : public Object {
public:
    Vector center;
    float size;
    float height;
    Versor rot;

    Paraboloid(Vector c, float s, float h, Material* mat, Versor v):Object(mat), center(c), size(s), height(h),rot(v) {

    }

    Ray to_local(Ray ray) {
        Ray r(ray.src-center, ray.dir); // translate

        r = r.rotate(rot); // rotate
        r.dir = r.dir.normalize();
        return r;
    }

    Vector inverse_transform(Vector v) {
        return rot.inverse().rotate(Vector(v.x, v.y, v.z)) + center;
    }

    Vector inverse_transform_normal(Vector normal) {
        return rot.inverse().rotate(Vector(normal.x, normal.y, normal.z)).normalize();
    }

    float inverse_transform(Ray r, float distance) {
        Vector i = r.along(distance);
        i = inverse_transform(i);
        Vector src = inverse_transform(r.src);
        return (i-src).length();
    }

    bool intersect(Ray ray, float* distance, Vector* normal) {
        ray = to_local(ray);
        Vector const & src = ray.src;
        Vector const & dir = ray.dir;

        float size2 = size * size;

        float a = pow(dir.x, 2) + pow(dir.y, 2);
        float b = 2.0 * ((src.x * dir.x) + (src.y * dir.y)) - size2 * dir.z;
        float c = pow(src.x, 2) + pow(src.y, 2) - size2 * src.z;

        float part_sol = b * b - (4.0 * a * c);

        if (part_sol >= 0)
        {
            float root1 = (-1 * b + sqrt(part_sol)) / (2.0 * a);
            float root2 = (-1 * b - sqrt(part_sol)) / (2.0 * a);

            float closer_root =  root1 < root2 ? root1 : root2;
            float farther_root = root1 > root2 ? root1 : root2;

            if (closer_root > 0)
            {
                Vector p = ray.along(closer_root);

                if (p.z > 0 && p.z < height) {
                    *distance = inverse_transform(ray, closer_root);
                    *normal = inverse_transform_normal(Vector(2 * p.x / size2, 2 * p.y / size2, -1.0f));
                    return true;
                }
            }

            if (farther_root > 0)
            {
                Vector p = ray.along(farther_root);

                if (p.z > 0 && p.z < height) {
                    *distance = inverse_transform(ray, farther_root);
                    *normal = inverse_transform_normal(Vector(2 * p.x / size2, 2 * p.y / size2, -1.0f) * -1.0f);
                    return true;
                }
            }
        }
        return false;
    }

};


class Scene {
public:
    Object* objects[10];
    Light lights[3];
    int objects_size;
    Scene() {
        objects_size = 0;
    }

    void render() {
        for(int y=0; y < screenHeight; y++) {
            for(int x=0; x < screenWidth; x++) {
                Ray r = camera.get_ray(x, y);
                image[y*screenWidth + x] = trace(r, 0);
            }
        }

    }

    Color trace(Ray ray, int d) {
        Color ambient_light = Color(135/255.0, 206/255.0, 250/255.0) * 0.9;
        if(d > 4) return ambient_light;

        float distance;
        Vector normal;
        Object* obj;

        if(!intersect_all(ray, &distance, &normal, &obj))
            return ambient_light;

        Vector point = ray.src + ray.dir * distance;

        Color sum_color = ambient_light;

        for(int i=0; i<3; i++) {
            Vector hit_to_light = (lights[i].point - point).normalize();
            float light_dist = 1.0/pow((lights[i].point - point).length(), 2);
            float cos_theta = normal * hit_to_light;
            Vector H = (hit_to_light - ray.dir).normalize();

            Ray shadow_ray = Ray(point, hit_to_light);
            float dd; Vector nn; Object* oo;
            if(!intersect_all(shadow_ray, &dd, &nn, &oo)) {
                Color light_color = lights[i].color * light_dist * lights[i].power;

                float NH = normal * H;
                NH = NH > 0 ? NH : 0;

                if (obj->material->is_diffuse) {
                    sum_color = sum_color + light_color;
                    // http://www.wikiwand.com/en/Blinn%E2%80%93Phong_shading_model#/Code_sample
                    float intensity = pow(NH, 10);

                    sum_color = sum_color + light_color * intensity;
                } else {
                    Color specular_color = obj->material->fresnel(cos_theta) * light_color * NH * cos_theta;
                    sum_color = sum_color + specular_color;
                }
            }
        }

        Color return_color = obj->material->get_color(point) * sum_color;

        if(obj->material->is_reflective) {
            Ray reflected_ray = Ray(point + normal * EPSILON, ray.dir - (normal * 2.0) * ( normal * ray.dir));

            float NR = normal * reflected_ray.dir;

            Color reflection_color = obj->material->fresnel(NR) * trace(reflected_ray, d + 1);
            return_color = return_color + reflection_color;
        }

        return return_color;
    }

    bool intersect_all(Ray ray, float* idistance, Vector* inormal, Object** iobject) {
        *idistance = FLOAT_MAX;

        float distance;
        Vector normal;

        for(int i=0; i<objects_size; i++) {
            if(objects[i]->intersect(ray, &distance, &normal) && distance < *idistance) {
                *idistance = distance;
                *inormal = normal;
                *iobject = objects[i];
            }
        }

        return *idistance < FLOAT_MAX;
    }

    void add_object(Object* obj) {
        objects[objects_size++] = obj;
    }

} scene;




// Inicializacio, a program futasanak kezdeten, az OpenGL kontextus letrehozasa utan hivodik meg (ld. main() fv.)
void onInitialization( ) {
	glViewport(0, 0, screenWidth, screenHeight);

        scene.lights[0] = Light(Vector(30, 4, 10), Color(1, 0, 0), 40);
        scene.lights[1] = Light(Vector(20, 0, 10), Color(0, 1, 0), 40);
        scene.lights[2] = Light(Vector(10, -4, 9), Color(0, 0, 1), 40);

        Ellipsoid ell1(Vector(20, 3, 2), 0.8, 2, &gold, Versor(Vector(1, 0, 0), 3.14/2.0*0));

        scene.add_object(&table1);
        scene.add_object(&table2);
        scene.add_object(&ell1);

        float distance;
        Vector normal;
        Ray ray(Vector(20, 2.4, 10), Vector(0, 0, -1));


        ell1.intersect(ray, &distance, &normal);
        Vector point = (ray.src + ray.dir * distance);
        point = point + normal * 1.2;
        Versor v((Vector(0, 0, 1) % normal), (acos(normal * Vector(0, 0, 1))) * -1.0);
        Ellipsoid ell2(point, 0.4f, 1.2f, &gold, v);
        scene.add_object(&ell2);

        ray = Ray(Vector(19.9, 1.72, 10), Vector(0, 0, -1));

        ell2.intersect(ray, &distance, &normal);
        point = Vector(ray.src + ray.dir * distance);
        point = point + normal * 1.0;
        v = Versor((Vector(0, 0, 1) % normal), (acos(normal * Vector(0, 0, 1))) * -1.0);
        Ellipsoid ell3(point, 0.2, 1, &gold, v);
        scene.add_object(&ell3);

//        Ellipsoid test(Vector(25, 0, 1), 1, 1.5, &gold, Versor(Vector(1, 0, 0), 3.14/2));
//        scene.add_object(&test);

        Paraboloid par(Vector(22, -3, 3), 0.8, 3, &silver, Versor(Vector(0, 1, 0), 3.14));

        float  para_distance;
        Vector para_normal;
        Ray    pa_ray(Vector(22, -2.0, 10), Vector(0, 0, -1));
        Vector para_point;

        par.intersect(pa_ray, &para_distance, &para_normal);
        para_point = pa_ray.src + pa_ray.dir * para_distance;

        Paraboloid par1(para_point, 0.4, 2, &silver, Versor(Vector(0, 0, 1) % para_normal, acos(Vector(0, 0, 1) * para_normal) * -1));


        pa_ray = Ray(Vector(22.2, -1, 10), Vector(0, 0, -1));
        par1.intersect(pa_ray, &para_distance, &para_normal);
        para_point = pa_ray.src + pa_ray.dir * para_distance;
        Paraboloid par2(para_point, 0.2, 1.5, &silver, Versor(Vector(0, 0, 1) % para_normal, acos(Vector(0, 0, 1) * para_normal) * -1));

        scene.add_object(&par);
        scene.add_object(&par1);
        scene.add_object(&par2);

        scene.render();
}

// Rajzolas, ha az alkalmazas ablak ervenytelenne valik, akkor ez a fuggveny hivodik meg
void onDisplay( ) {
    glClearColor(0.1f, 0.2f, 0.3f, 1.0f);		// torlesi szin beallitasa
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // kepernyo torles

    // Peldakent atmasoljuk a kepet a rasztertarba
    glDrawPixels(screenWidth, screenHeight, GL_RGB, GL_FLOAT, image);

    glutSwapBuffers();     				// Buffercsere: rajzolas vege

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
void onMouseMotion(int x, int y)
{

}

// `Idle' esemenykezelo, jelzi, hogy az ido telik, az Idle esemenyek frekvenciajara csak a 0 a garantalt minimalis ertek
void onIdle( ) {
     //long time = glutGet(GLUT_ELAPSED_TIME);		// program inditasa ota eltelt ido
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

/*
 ============================================================================
 Name        : raycast.c
 Author      : Anthony Black
 Description : CS430 Project 2: Raycaster
 ============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
//STRUCTURES
// Plymorphism in C
  
typedef struct {
  double width;
  double height;
}Camera;

typedef struct {
  int type; // 0 = sphere, 1 = plane
  double color[3];
  double position[3];
  union {
    struct {
      double radius;
    } sphere;
    struct {
      double normal[3];
    } plane;
  };
} Object;
//PROTOTYPE DECLARATIONS

int next_c(FILE* json);

void expect_c(FILE* json, int d);

void skip_ws(FILE* json);

char* next_string(FILE* json);

double next_number(FILE* json);

double* next_vector(FILE* json);

void read_scene(char* filename, Camera camera, Object** objects);

double sqr(double v);

void normalize(double* v);

double sphere_intersection(double* Ro, double* Rd, double* C, double r);

int line = 1;

int main(int argc, char *argv[]) {
    Object** objects;
    objects = malloc(sizeof(Object*)*128);
    Camera camera;
    read_scene(argv[1], camera, objects);
    printf("Made it  here.\n");
    printf("Camera width: %f.\n", camera.width);
    printf("Camera height: %f.\n", camera.height);
    int k = 0;
    while(objects[k] != 0){
      if(objects[k]->type == 0){
        printf("Sphere color: [%f, %f, %f].\n", objects[k]->color[0], objects[k]->color[1], objects[k]->color[2]);
        printf("Sphere position: [%f, %f, %f].\n", objects[k]->position[0], objects[k]->position[1], objects[k]->position[2]);
        printf("Sphere radius: %f.\n", objects[k]->sphere.radius);
      }
      else if(objects[k]->type == 0){
        printf("Plane color: [%f, %f, %f].\n", objects[k]->color[0], objects[k]->color[1], objects[k]->color[2]);
        printf("Plane position: [%f, %f, %f].\n", objects[k]->position[0], objects[k]->position[1], objects[k]->position[2]);
        printf("Plane normal: [%f, %f, %f].\n", objects[k]->plane.normal[0], objects[k]->plane.normal[1], objects[k]->plane.normal[2]);
      }
      else{
        printf("Unknown entry.\n");
      }
      k++;
    }
    printf("Finished printing objects.\n");
    //generate_scene();
    return EXIT_SUCCESS;
}

int next_c(FILE* json) {
  // next_c() wraps the getc() function and provides error checking and line
  // number maintenance
  int c = fgetc(json);
  #ifdef DEBUG
    printf("next_c: '%c'\n", c);
  #endif
  if (c == '\n') {
    line += 1;
  }
  if (c == EOF) {
    fprintf(stderr, "Error: Unexpected end of file on line number %d.\n", line);
    exit(1);
  }
  return c;
}


void expect_c(FILE* json, int d) {
  // expect_c() checks that the next character is d.  If it is not it emits
  // an error.
  int c = next_c(json);
  if (c == d) return;
  fprintf(stderr, "Error: Expected '%c' on line %d.\n", d, line);
  exit(1);    
}


void skip_ws(FILE* json) {
  // skip_ws() skips white space in the file.
  int c = next_c(json);
  while (isspace(c)) {
    c = next_c(json);
  }
  ungetc(c, json);
}


char* next_string(FILE* json) {
  // next_string() gets the next string from the file handle and emits an error
  // if a string can not be obtained. 
  char buffer[129];
  int c = next_c(json);
  if (c != '"') {
    fprintf(stderr, "Error: Expected string on line %d.\n", line);
    exit(1);
  }  
  c = next_c(json);
  int i = 0;
  while (c != '"') {
    if (i >= 128) {
      fprintf(stderr, "Error: Strings longer than 128 characters in length are not supported.\n");
      exit(1);      
    }
    if (c == '\\') {
      fprintf(stderr, "Error: Strings with escape codes are not supported.\n");
      exit(1);      
    }
    if (c < 32 || c > 126) {
      fprintf(stderr, "Error: Strings may contain only ascii characters.\n");
      exit(1);
    }
    buffer[i] = c;
    i += 1;
    c = next_c(json);
  }
  buffer[i] = 0;
  return strdup(buffer);
}

double next_number(FILE* json) {
  double value;
  fscanf(json, "%lf", &value);
  // Error check this..
  return value;
}

double* next_vector(FILE* json) {
  double* v = malloc(3*sizeof(double));
  expect_c(json, '[');
  skip_ws(json);
  v[0] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[1] = next_number(json);
  skip_ws(json);
  expect_c(json, ',');
  skip_ws(json);
  v[2] = next_number(json);
  skip_ws(json);
  expect_c(json, ']');
  return v;
}

void read_scene(char* filename, Camera camera, Object** objects) {
  int c;
  int current_item = -1; //for tracking the current object in the Object array
  int current_type; //for tracking the current object we are reading from the json list
  FILE* json = fopen(filename, "r");
  //if file does not exist
  if (json == NULL) {
    fprintf(stderr, "Error: Could not open file \"%s\"\n", filename);
    exit(1);
  }
  
  skip_ws(json);
  
  // Find the beginning of the list
  expect_c(json, '[');

  skip_ws(json);

  // Find the objects

  while (1) {
    c = fgetc(json);
    if (c == ']') {
      fprintf(stderr, "Error: This is the worst scene file EVER.\n");
      fclose(json);
      return;
    }
    if (c == '{') {
      skip_ws(json);
    
      // Parse the object
      char* key = next_string(json);
      if (strcmp(key, "type") != 0) {
    fprintf(stderr, "Error: Expected \"type\" key on line number %d.\n", line);
    exit(1);
      }

      skip_ws(json);

      expect_c(json, ':');

      skip_ws(json);

      char* value = next_string(json);

      if (strcmp(value, "camera") == 0) {
        printf("Current object: camera\n");
        if(camera.width > 0.001 || camera.height > 0.001){
          fprintf(stderr, "Width %f, height %f\n", camera.width, camera.height);
          fprintf(stderr, "Error: Second camera object in json on line number %d.\n", line);
          exit(1);
        }
        current_type = 0;
      } 
      else if (strcmp(value, "sphere") == 0) {
        printf("Current object: sphere\n");
        current_item++;
        objects[current_item] = malloc(sizeof(Object));
        objects[current_item]->type = 0;
        current_type = 1;
      } 
      else if (strcmp(value, "plane") == 0) {
        printf("Current object: plane\n");
        current_item++;
        objects[current_item]->type = 1;
        current_type = 2;
      } 
      else { 
        fprintf(stderr, "Error: Unknown type, \"%s\", on line number %d.\n", value, line);
        exit(1);
      }

      skip_ws(json);

      while (1) {
        // , }
        c = next_c(json);
        if (c == '}') {
          // stop parsing this object
          break;
        } 
        else if (c == ',') {
          // read another field
          skip_ws(json);
          char* key = next_string(json);
          skip_ws(json);
          expect_c(json, ':');
          skip_ws(json);
          if (strcmp(key, "width") == 0){
            printf("Read type: width\n");
            if(current_type == 0){  //only camera has width
              if(camera.width < 0.001){ //makes sure we don't have a pre-existing camera
                camera.width = next_number(json);  
              }
              else{
                fprintf(stderr, "Error: Second camera width value detected on line number %d.\n", line);
                exit(1);
              }
            }
            else{
              fprintf(stderr, "Error: Current object type has width value on line number %d.\n", line);
              exit(1);
            }
          }
          else if(strcmp(key, "height") == 0){
            printf("Read type: height\n");
            if(current_type == 0){  //only camera has height
              if(camera.height < 0.001){ //makes sure we don't have a pre-existing camera
                camera.height = next_number(json);  
              }
              else{
                fprintf(stderr, "Error: Second camera height value detected on line number %d.\n", line);
                exit(1);
              }
            }
            else{
              fprintf(stderr, "Error: Current object type has height value on line number %d.\n", line);
              exit(1);
            }
          }
          else if(strcmp(key, "radius") == 0){
            printf("Read type: radius\n");
            if(current_type == 1){  //only spheres have radius
              if(objects[current_item]->sphere.radius == 0){ //makes sure we don't have a pre-existing radius
                objects[current_item]->sphere.radius = next_number(json);  
              }
              else{
                fprintf(stderr, "Error: Second radius found for current object! Detected on line number %d.\n", line);
                exit(1);  
              }
            }
            else{
              fprintf(stderr, "Error: Current object type cannot have radius value! Detected on line number %d.\n", line);
              exit(1);
            }
          }     
          else if(strcmp(key, "color") == 0){
            printf("Read type: color\n");
            if(current_type == 1 || current_type == 2){  //only spheres and planes have color
              if(objects[current_item]->color == 0){ //makes sure we don't have a pre-existing camera
                double* vector = next_vector(json);
                objects[current_item]->color[0] = (*vector++);
                objects[current_item]->color[1] = (*vector++);
                objects[current_item]->color[2] = (*vector++);  
              }
              else{
                fprintf(stderr, "Error: Second object color value detected on line number %d.\n", line);
                exit(1);
              }
            }
            else{
              fprintf(stderr, "Error: Camera type has color value on line number %d.\n", line);
              exit(1);
            }
          } 
          else if(strcmp(key, "position") == 0){
            printf("Read type: position\n");
            if(current_type == 1 || current_type == 2){  //only spheres and planes have position
              if(objects[current_item]->position == 0){ //makes sure we don't have a pre-existing camera
                double* vector = next_vector(json);
                objects[current_item]->position[0] = (*vector++);
                objects[current_item]->position[1] = (*vector++);
                objects[current_item]->position[2] = (*vector++);  
              }
              else{
                fprintf(stderr, "Error: Second object position value detected on line number %d.\n", line);
                exit(1);
              }
            }
            else{
              fprintf(stderr, "Error: Camera type has position value on line number %d.\n", line);
              exit(1);
            }
          } 
          else if(strcmp(key, "normal") == 0){
            printf("Read type: normal\n");
            if(current_type == 2){  //only planes have normal
              if(objects[current_item]->plane.normal == 0){ //makes sure we don't have a pre-existing camera
                double* vector = next_vector(json);
                objects[current_item]->plane.normal[0] = (*vector++);
                objects[current_item]->plane.normal[1] = (*vector++);
                objects[current_item]->plane.normal[2] = (*vector++);    
              }
              else{
                fprintf(stderr, "Error: Second normal value detected on line number %d.\n", line);
                exit(1);
              }
            }
            else{
              fprintf(stderr, "Error: Only planes have normal values on line number %d.\n", line);
              exit(1);
            }
          } 
          else{
            fprintf(stderr, "Error: Unknown property, \"%s\", on line %d.\n",
                key, line);
            //char* value = next_string(json);
          }

          skip_ws(json);
        } 
        else {
          fprintf(stderr, "Error: Unexpected value on line %d\n", line);
          exit(1);
        }
      }

      skip_ws(json);

      c = next_c(json);

      if (c == ',') {
        // noop
        skip_ws(json);
      } 
      else if (c == ']') {
        fclose(json);
        return;
      } 
      else {
        fprintf(stderr, "Error: Expecting ',' or ']' on line %d.\n", line);
        exit(1);
      }
    }
  }
}

double sqr(double v) {
  return v*v;
}

void normalize(double* v) {
  double len = sqrt(sqr(v[0]) + sqr(v[1]) + sqr(v[2]));
  v[0] /= len;
  v[1] /= len;
  v[2] /= len;
}

double sphere_intersection(double* Ro, double* Rd, double* C, double r) {
  // Step 1. Find the equation for the object you are
  // interested in..  (e.g., sphere)
  //
  // x^2 + y^2 + z^2 = r^2
  //
  // Step 2. Parameterize the equation with a center point
  // if needed
  //
  // (x-Cx)^2 + (y-Cy)^2 + (z-Cz)^2 = r^2
  //
  // Step 3. Substitute the eq for a ray into our object
  // equation.
  //
  // (Rox + t*Rdx - Cx)^2 + (Roy + t*Rdy - Cy)^2 + (Roz + t*Rdz - Cz)^2 - r^2 = 0
  //
  // Step 4. Solve for t.
  //
  // Step 4a. Rewrite the equation (flatten).
  //
  // -r^2 +
  // t^2 * Rdx^2 +
  // t^2 * Rdy^2 +
  // t^2 * Rdz^2 +
  // 2*t * Rox * Rdx -
  // 2*t * Rdx * Cx +
  // 2*t * Roy * Rdy -
  // 2*t * Rdy * Cy +
  // 2*t * Roz * Rdz -
  // 2*t * Rdz * Cz +
  // Rox^2 -
  // 2*Rox*Cx +
  // Cx^2 +
  // Roy^2 -
  // 2*Roy*Cy +
  // Cy^2 +
  // Roz^2 -
  // 2*Roz*Cz +
  // Cz^2 = 0
  //
  // Steb 4b. Rewrite the equation in terms of t.
  //
  // t^2 * (Rdx^2 + Rdy^2 + Rdz^2) +
  // t * (2 * (Rox * Rdx - Rdx * Cx + Roy * Rdy - Rdy *Cy Roz * Rdz - Rdz * Cz)) +
  // Rox^2 - 2*Rox*Cx + Cx^2 + Roy^2 - 2*Roy*Cy + Cy^2  + Roz^2 - 2*Roz*Cz + Cz^2 - r^2 = 0
  //
  // Use the quadratic equation to solve for t..
  double a = (sqr(Rd[0]) + sqr(Rd[1]) + sqr(Rd[2]));
  double b = (2 * (Ro[0] * Rd[0] - Rd[0] * C[0] + Ro[1] * Rd[1] - Rd[1] * C[1] + Ro[2] * Rd[2] - Rd[2] * C[2]));
  double c = sqr(Ro[0]) - 2*Ro[0]*C[0] + sqr(C[0]) + sqr(Ro[1]) - 2*Ro[1]*C[1] + sqr(C[1]) + sqr(Ro[2]) - 2*Ro[2]*C[2] + sqr(C[2]) - sqr(r);

  double det = sqr(b) - 4 * a * c;
  if (det < 0) return -1;

  det = sqrt(det);

  double t0 = (-b - det) / (2*a);
  if (t0 > 0) return t0;

  double t1 = (-b + det) / (2*a);
  if (t1 > 0) return t1;

  return -1;
}

void generate_scene(Camera camera, Object** objects){
    //write these objects to a ppm image
    int w = 400;
    int h = 400;
    double M = camera.height;
    double N = camera.width;
    double pixheight = h / M;
          double pixwidth = w / N;
          for (int y = 0; y < M; y += 1) {
            for (int x = 0; x < N; x += 1) {
              double Ro[3] = {0, 0, 0};
              // Rd = normalize(P - Ro)
              double Rd[3] = {
                0 - (w/2) + pixwidth * (x + 0.5),
                0 - (h/2) + pixheight * (y + 0.5),
                1
              };
              normalize(Rd);

              double best_t = INFINITY;
              for (int i=0; objects[i] != 0; i += 1) {
            double t = 0;

            switch(objects[i]->type) {
            case 0:
              t = sphere_intersection(Ro, Rd,
                            objects[i]->position,
                            objects[i]->sphere.radius);
              break;
            case 1:
            default:
              // Horrible error
              exit(1);
            }
            if (t > 0 && t < best_t) best_t = t;
              }
              if (best_t > 0 && best_t != INFINITY) {
            printf("#");
              } else {
            printf(".");
              }

            }
            printf("\n");
          }
}


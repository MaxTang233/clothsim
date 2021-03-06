#include <iostream>
#include <math.h>
#include <random>
#include <vector>

#include "cloth.h"
#include "collision/plane.h"
#include "collision/sphere.h"

using namespace std;

Cloth::Cloth(double width, double height, int num_width_points,
             int num_height_points, float thickness) {
  this->width = width;
  this->height = height;
  this->num_width_points = num_width_points;
  this->num_height_points = num_height_points;
  this->thickness = thickness;

  buildGrid();
  buildClothMesh();
}

Cloth::~Cloth() {
  point_masses.clear();
  springs.clear();

  if (clothMesh) {
    delete clothMesh;
  }
}

void Cloth::buildGrid() {
  // TODO (Part 1): Build a grid of masses and springs.
    
    
  
    
    if(orientation == HORIZONTAL){
        float z_step = height / (num_height_points - 1.);
        float x_step = width / (num_width_points - 1.);
        for(int z = 0; z < num_height_points; z++){
            for(int x = 0; x < num_width_points; x ++){
  
                bool pin = false;
                for(const auto point: this->pinned){

                    if(point[0] == x && point[1] == z){
                        pin = true;
                        break;
                    }
                }
//                cout << pin;
          

                auto p = PointMass(Vector3D(x_step * x, 1., z_step * z),pin);
                point_masses.emplace_back(p);
            }
        }
    }else{
        
     
        float y_step = height / (num_height_points-1.);
        float x_step = width / (num_width_points-1.);

        for(int y = 0; y < num_height_points; y++){
            for(int x = 0; x < num_width_points; x ++){
                auto z = (1. * rand() / RAND_MAX - 1.) / 1000.;
                bool pin = false;
                for(const auto point: this->pinned){
                    if(point[0] == x && point[1] == y){
                        pin = true;
                        break;
                    }
                }
    
//                cout << z;
                
                auto p = PointMass(Vector3D(x_step * x, y_step * y, z),pin);
                point_masses.emplace_back(p);
            }
        }
        
    }
    
    
    for(int row = 0; row < num_height_points; row ++){
        for(int col = 0; col < num_width_points; col ++){
        //Structural constraints exist between a point mass and the point mass to its left as well as the point mass above it.
            //left
            if(col > 0){
                Spring s = Spring(&point_masses[row * num_width_points + col], &point_masses[row * num_width_points + col -1 ], STRUCTURAL);
                springs.emplace_back(s);
                
            }
            //above
            if(row > 0){
                Spring s = Spring(&point_masses[row * num_width_points + col], &point_masses[(row-1) * num_width_points + col], STRUCTURAL);
                springs.emplace_back(s);
                //Shearing constraints exist between a point mass and the point mass to its diagonal upper left as well as the point mass to its diagonal upper right.
                //upper left
                if(col > 0){
                    Spring s = Spring(&point_masses[row * num_width_points + col], &point_masses[(row-1) * num_width_points + col - 1], SHEARING);
                    springs.emplace_back(s);
                    
                }
                //upper right
                if(col < num_width_points - 1){
                    Spring s = Spring(&point_masses[row * num_width_points + col], &point_masses[(row-1) * num_width_points + col + 1], SHEARING);
                    springs.emplace_back(s);
                    
                }
            }
            //Bending constraints exist between a point mass and the point mass two away to its left as well as the point mass two above it.
            //two away to left
            if(col > 1){
                Spring s = Spring(&point_masses[row * num_width_points + col], &point_masses[row * num_width_points + col - 2], BENDING);
                springs.emplace_back(s);
                
            }
            //two away above
            if(row > 1){
                Spring s = Spring(&point_masses[row * num_width_points + col], &point_masses[(row - 2) * num_width_points + col], BENDING);
                springs.emplace_back(s);
                
            }

        }
        
    }


}

void Cloth::simulate(double frames_per_sec, double simulation_steps, ClothParameters *cp,
                     vector<Vector3D> external_accelerations,
                     vector<CollisionObject *> *collision_objects) {
    
  double mass = width * height * cp->density / num_width_points / num_height_points;
  double delta_t = 1.0f / frames_per_sec / simulation_steps;

  // TODO (Part 2): Compute total force acting on each point mass.

    
    Vector3D F = Vector3D(0,0,0);
    for(const auto a: external_accelerations){
        F += a * mass;
    }
    for(auto &p:point_masses){
        p.forces = F;

    }

  
    for(Spring &s: springs){
//        enum e_spring_type { STRUCTURAL = 0, SHEARING = 1, BENDING = 2 };
        if((!cp->enable_structural_constraints && s.spring_type == 0) || (!cp->enable_shearing_constraints && s.spring_type == 1)|| (!cp->enable_bending_constraints && s.spring_type == 2)){
            
            continue;
            
        }else{
//            cout << "1";
            
            auto Fs = (s.pm_a->position - s.pm_b -> position).unit() * cp->ks * ((s.pm_a->position - s.pm_b->position).norm() - s.rest_length);
            if(s.spring_type == 2){
                Fs *= .2;
            }
            s.pm_a->forces -= Fs;
            s.pm_b->forces += Fs;
            
        }
        
    }


  // TODO (Part 2): Use Verlet integration to compute new point mass positions

    
    for(auto &p:point_masses){
//         cout << "2";

        if(!p.pinned){

        Vector3D at = p.forces/mass;
//            cout << p.forces;
            
        Vector3D new_pos = p.position + (1. - (double)cp->damping / 100) * (p.position - p.last_position) + at * delta_t * delta_t;
        p.last_position = p.position;

        p.position = new_pos;

            
        }
    }
    
    // TODO (Part 4): Handle self-collisions.
        build_spatial_map();
//     TODO (Part 3): Handle collisions with other primitives.
      
        for(auto &p:point_masses){
            self_collide(p, simulation_steps);
            for(auto obj: *collision_objects){
              obj->collide(p);
          }
//            cout << p.position.z;
            
      }
    
        // TODO (Part 2): Constrain the changes to be such that the spring does not change
        // in length more than 10% per timestep [Provot 1995].
        for(Spring &s: springs){
    //        enum e_spring_type { STRUCTURAL = 0, SHEARING = 1, BENDING = 2 };
           
            double diff = (s.pm_a->position - s.pm_b -> position).norm() - s.rest_length * 1.1;
        
            if(diff > 0){
                //correction should be a vector on the pm_a-pm_b line
                Vector3D correction = diff *.5 * (s.pm_a->position - s.pm_b -> position).unit();
                
                if(s.pm_a->pinned && s.pm_b->pinned){
                    continue;
                }
                
                if (s.pm_a->pinned && !s.pm_b->pinned){
                    s.pm_b->position += 2.*correction;
                }else if (!s.pm_a->pinned && s.pm_b->pinned){
                    s.pm_a->position -= 2.*correction;
                }else{
//                    cout << "3";
                    s.pm_a->position -= correction;
                    s.pm_b->position += correction;
                    
                }
            
            }
            
        }


}

void Cloth::build_spatial_map() {
  for (const auto &entry : map) {
    delete(entry.second);
  }
  map.clear();

  // TODO (Part 4): Build a spatial map out of all of the point masses.
    for(auto &p:point_masses){
        float key = hash_position(p.position);
        if(!map[key]){
            map[key] = new vector<PointMass *>();
        }
        map[key]->push_back(&p);
    }
}

void Cloth::self_collide(PointMass &pm, double simulation_steps) {
  // TODO (Part 4): Handle self-collision for a given point mass.
    Vector3D correction;
    float key = hash_position(pm.position);
    int num = 0;
    if(map[key]){
        for(auto *p:*map[key]){
        
            if(p != &pm){
                auto vec = pm.position -p->position;
                auto dis = vec.norm();
                if(dis < thickness * 2.){
                    num += 1;
                    correction += vec.unit() * (2.*thickness - dis);
                }
              
            }
        }
    }

    if(num != 0){
       pm.position += correction /num / simulation_steps;
    }
    

}

float Cloth::hash_position(Vector3D pos) {
  // TODO (Part 4): Hash a 3D position into a unique float identifier that represents membership in some 3D box volume.
    auto w = 3. * width / num_width_points;
    auto h = 3. * height / num_height_points;
    auto t = max(w,h);
    
    auto x = floor(pos.x / w);
    auto y = floor(pos.y / h);
    auto z = floor(pos.z / t);
    
    //Flat[x + WIDTH * (y + DEPTH * z)] = Original[x, y, z]
  return x + width/3. * (y * height/3. + z);
}

///////////////////////////////////////////////////////
/// YOU DO NOT NEED TO REFER TO ANY CODE BELOW THIS ///
///////////////////////////////////////////////////////

void Cloth::reset() {
  PointMass *pm = &point_masses[0];
  for (int i = 0; i < point_masses.size(); i++) {
    pm->position = pm->start_position;
    pm->last_position = pm->start_position;
    pm++;
  }
}

void Cloth::buildClothMesh() {
  if (point_masses.size() == 0) return;

  ClothMesh *clothMesh = new ClothMesh();
  vector<Triangle *> triangles;

  // Create vector of triangles
  for (int y = 0; y < num_height_points - 1; y++) {
    for (int x = 0; x < num_width_points - 1; x++) {
      PointMass *pm = &point_masses[y * num_width_points + x];
      // Get neighboring point masses:
      /*                      *
       * pm_A -------- pm_B   *
       *             /        *
       *  |         /   |     *
       *  |        /    |     *
       *  |       /     |     *
       *  |      /      |     *
       *  |     /       |     *
       *  |    /        |     *
       *      /               *
       * pm_C -------- pm_D   *
       *                      *
       */
      
      float u_min = x;
      u_min /= num_width_points - 1;
      float u_max = x + 1;
      u_max /= num_width_points - 1;
      float v_min = y;
      v_min /= num_height_points - 1;
      float v_max = y + 1;
      v_max /= num_height_points - 1;
      
      PointMass *pm_A = pm                       ;
      PointMass *pm_B = pm                    + 1;
      PointMass *pm_C = pm + num_width_points    ;
      PointMass *pm_D = pm + num_width_points + 1;
      
      Vector3D uv_A = Vector3D(u_min, v_min, 0);
      Vector3D uv_B = Vector3D(u_max, v_min, 0);
      Vector3D uv_C = Vector3D(u_min, v_max, 0);
      Vector3D uv_D = Vector3D(u_max, v_max, 0);
      
      
      // Both triangles defined by vertices in counter-clockwise orientation
      triangles.push_back(new Triangle(pm_A, pm_C, pm_B, 
                                       uv_A, uv_C, uv_B));
      triangles.push_back(new Triangle(pm_B, pm_C, pm_D, 
                                       uv_B, uv_C, uv_D));
    }
  }

  // For each triangle in row-order, create 3 edges and 3 internal halfedges
  for (int i = 0; i < triangles.size(); i++) {
    Triangle *t = triangles[i];

    // Allocate new halfedges on heap
    Halfedge *h1 = new Halfedge();
    Halfedge *h2 = new Halfedge();
    Halfedge *h3 = new Halfedge();

    // Allocate new edges on heap
    Edge *e1 = new Edge();
    Edge *e2 = new Edge();
    Edge *e3 = new Edge();

    // Assign a halfedge pointer to the triangle
    t->halfedge = h1;

    // Assign halfedge pointers to point masses
    t->pm1->halfedge = h1;
    t->pm2->halfedge = h2;
    t->pm3->halfedge = h3;

    // Update all halfedge pointers
    h1->edge = e1;
    h1->next = h2;
    h1->pm = t->pm1;
    h1->triangle = t;

    h2->edge = e2;
    h2->next = h3;
    h2->pm = t->pm2;
    h2->triangle = t;

    h3->edge = e3;
    h3->next = h1;
    h3->pm = t->pm3;
    h3->triangle = t;
  }

  // Go back through the cloth mesh and link triangles together using halfedge
  // twin pointers

  // Convenient variables for math
  int num_height_tris = (num_height_points - 1) * 2;
  int num_width_tris = (num_width_points - 1) * 2;

  bool topLeft = true;
  for (int i = 0; i < triangles.size(); i++) {
    Triangle *t = triangles[i];

    if (topLeft) {
      // Get left triangle, if it exists
      if (i % num_width_tris != 0) { // Not a left-most triangle
        Triangle *temp = triangles[i - 1];
        t->pm1->halfedge->twin = temp->pm3->halfedge;
      } else {
        t->pm1->halfedge->twin = nullptr;
      }

      // Get triangle above, if it exists
      if (i >= num_width_tris) { // Not a top-most triangle
        Triangle *temp = triangles[i - num_width_tris + 1];
        t->pm3->halfedge->twin = temp->pm2->halfedge;
      } else {
        t->pm3->halfedge->twin = nullptr;
      }

      // Get triangle to bottom right; guaranteed to exist
      Triangle *temp = triangles[i + 1];
      t->pm2->halfedge->twin = temp->pm1->halfedge;
    } else {
      // Get right triangle, if it exists
      if (i % num_width_tris != num_width_tris - 1) { // Not a right-most triangle
        Triangle *temp = triangles[i + 1];
        t->pm3->halfedge->twin = temp->pm1->halfedge;
      } else {
        t->pm3->halfedge->twin = nullptr;
      }

      // Get triangle below, if it exists
      if (i + num_width_tris - 1 < 1.0f * num_width_tris * num_height_tris / 2.0f) { // Not a bottom-most triangle
        Triangle *temp = triangles[i + num_width_tris - 1];
        t->pm2->halfedge->twin = temp->pm3->halfedge;
      } else {
        t->pm2->halfedge->twin = nullptr;
      }

      // Get triangle to top left; guaranteed to exist
      Triangle *temp = triangles[i - 1];
      t->pm1->halfedge->twin = temp->pm2->halfedge;
    }

    topLeft = !topLeft;
  }

  clothMesh->triangles = triangles;
  this->clothMesh = clothMesh;
}

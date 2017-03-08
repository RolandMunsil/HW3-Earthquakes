#ifndef Earth_h
#define Earth_h

#include "GLSLProgram.h"
#include "config.h"
#include "Texture.h"
#include "Mesh.h"
#define GLM_FORCE_RADIANS
#include <glm/glm/glm.hpp>
#include <glm/glm/gtc/constants.hpp>

namespace basicgraphics{
class Earth {
public:
  Earth();
    ~Earth(){};
  
  // Given latitude and longitude, calculate 3D position
    glm::vec3 getPosition(double latitude, double longitude);
  
  // Calculate and set up the buffers to render to screen
  void setupGeometry();
  
  // Draw the Earth to screen
  void draw(GLSLProgram &shader);
    
protected:
  
  // Stores the earth texture data
    std::vector< std::shared_ptr<Texture> > textures;
    
	std::shared_ptr<Mesh> mesh;
};
}


#endif
  
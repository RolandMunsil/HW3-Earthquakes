#include "Earth.h"

namespace basicgraphics{

using namespace std;
using namespace glm;

Earth::Earth() {

    // Note: TEXTURE_PATH is set in config.h
    shared_ptr<Texture> tex = Texture::create2DTextureFromFile(TEXTURE_PATH);
    tex->setTexParameteri(GL_TEXTURE_WRAP_S, GL_REPEAT);
    tex->setTexParameteri(GL_TEXTURE_WRAP_T, GL_REPEAT);
    tex->setTexParameteri(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    tex->setTexParameteri(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    textures.push_back(tex);
	setupGeometry();
}

void Earth::setupGeometry() {
    // TODO: Write your own code here to tesselate and create texture coordinates for the Earth
    // and then save the data to a mesh (i.e. VBO). The tesselation
    // should be based on the STACKS and SLICES parameters.
    const int STACKS = 20;
    const int SLICES = 40;

	std::vector<Mesh::Vertex> cpuVertexArray;
	std::vector<int> cpuIndexArray;
	
	int thisVertexIndex = -1;

	//NORTH pole is at y= 1;
	//SOUTH pole is at y=-1;

	//STACK=0 is the top
	//STACK=STACKS-1 is the bottom

	//LONG=0 is at x=1,z=0
	//LONG=-90 is at x=0, z=1
	//LONG=+/-180 is at x=-1, z=0
	//LONG=90 is at x=0, z=-1

	//Texture coordinates are from the top-left

	
	for (int stack = 0; stack < STACKS; stack++) {

		int stackStartIndex = thisVertexIndex + 1;
		for (int slice = 0; slice < SLICES + 1; slice++) {

			vec3 pt;
			if (stack == 0) {
				pt = vec3(0, 1, 0);
			}
			else if (stack == STACKS - 1) {
				pt = vec3(0, -1, 0);
			}
			else {
				//Calc lat/long and get point
				double lat = -((stack / (double)STACKS) - .5) * pi<double>();
				double longitude = ((slice / (double)SLICES) - .5) * two_pi<double>();
				pt = getPosition(lat, longitude);
			}
	
			Mesh::Vertex vertex;
			vertex.position = pt;
			vertex.normal = normalize(pt);
			vertex.texCoord0 = vec2(slice / (double)SLICES, stack / (double)STACKS);
			cpuVertexArray.push_back(vertex);
			thisVertexIndex++;
	
			if(stack > 0) {
				int indexVertexAbove = thisVertexIndex - (SLICES + 1);
				int indexNextVertexOnSameLayer = stackStartIndex + ((thisVertexIndex - stackStartIndex + 1) % (SLICES + 1));
				int indexNextVertexOnLayerAbove = indexNextVertexOnSameLayer - (SLICES + 1);

				cpuIndexArray.push_back(indexVertexAbove);
				cpuIndexArray.push_back(thisVertexIndex);
				cpuIndexArray.push_back(indexNextVertexOnSameLayer);

				cpuIndexArray.push_back(indexVertexAbove);
				cpuIndexArray.push_back(indexNextVertexOnSameLayer);
				cpuIndexArray.push_back(indexNextVertexOnLayerAbove);
			}
		}
	}
	
	const int cpuVertexByteSize = sizeof(Mesh::Vertex) * cpuVertexArray.size();
	const int cpuIndexByteSize = sizeof(int) * cpuIndexArray.size();
	
	mesh.reset(new Mesh(textures, GL_TRIANGLES, GL_STATIC_DRAW, cpuVertexByteSize,
		cpuIndexByteSize, 0, cpuVertexArray, cpuIndexArray.size(), cpuIndexByteSize, &(cpuIndexArray[0])));
}

glm::vec3 Earth::getPosition(double latitude, double longitude) {
    
	return vec3(
		cos(latitude)*cos(longitude), 
		sin(latitude), 
		-cos(latitude)*sin(longitude));
}


void Earth::draw(GLSLProgram &shader) {
	mesh->draw(shader);
}
}
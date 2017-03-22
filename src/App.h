#ifndef App_h
#define App_h

#include "BaseApp.h"
#include "EarthquakeDatabase.h"
#include "Earth.h"
#include <glad/glad.h>
#include <glfw/glfw3.h>
#include <string.h>
#include <glm/glm/gtc/matrix_transform.hpp>


namespace basicgraphics {
class App : public BaseApp {
public:
  
    App(int argc, char** argv, std::string windowName, int windowWidth, int windowHeight);
    ~App();
  

    void onSimulation(double rdt);
    void onRenderGraphics() override;
    void onEvent(std::shared_ptr<Event> event) override;

  
protected:
    
    void initializeText(); // Initialized the font rendering library
    void drawText(); // Draws the current time on the screen.
	void drawString(const std::string text, float latitude, float longitude, mat4 view, mat4 projection);
    struct FONScontext* fs;
    GLSLProgram _textShader;
    
    double lastTime;
    glm::vec3 eye_world;
    bool mouseDown; // Signifies whether the left mouse button is currently held down.
    glm::vec2 lastMousePos;

  double currentTime;
  bool playing;
  double playbackScale;
  
  // Database through which you can access the earthquakes
  EarthquakeDatabase eqd;
  
  // Object representing the earth
    std::shared_ptr<Earth> earth;
	std::unique_ptr<Sphere> earthquakeVisualRed;
	std::unique_ptr<Sphere> earthquakeVisualOrange;
	std::unique_ptr<Sphere> earthquakeVisualYellow;

  // Rotation matrix for the earth and displayed earthquakes
    glm::mat4 rotation;
  
};
}
#endif
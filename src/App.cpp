#include "App.h"
#include "config.h"
#include "EarthquakeDatabase.h"
#include <iostream>
#include <sstream>
#include <fstream>

namespace basicgraphics{
    
#define FONTSTASH_IMPLEMENTATION
#include "fontstash.h"
#define GLFONTSTASH_IMPLEMENTATION
#include "glfontstash.h"
    
const int PLAYBACK_WINDOW = 365 * 24 * 60 * 60; // Year in seconds

using namespace std;
using namespace glm;

//POSSIBLE ADDITIONAL FEATURES:
//expand time window/show all earthquakes
//make 3d text next to earthquakes
//solar system
//alternate earth textures - heatmap, city lights, etc.
//smooth acceleration of rotation

App::App(int argc, char** argv, std::string windowName, int windowWidth, int windowHeight) : BaseApp(argc, argv, windowName, windowWidth, windowHeight) {

    lastTime = glfwGetTime();
    glClearColor(0.1f, 0.12f, 0.15f, 1.0f);
    
    eye_world = glm::vec3(0, 0, 4);
    mouseDown = false;
    
    // Check that the config.h paths exist
    std::ifstream infile(TEXTURE_PATH);
    if (!infile.good()) {
        std::cout<< "ERROR: Unable to load the TEXTURE_PATH: \n\t"<< TEXTURE_PATH<<std::endl;
        std::cout << "\t Check that the path exists and modify config.h if needed"<<std::endl;
        assert(false);
    }
    else {
        infile.close();
    }
    std::ifstream infile2(DATA_PATH);
    if (!infile2.good()) {
        std::cout<< "ERROR: Unable to load the DATA_PATH: \n\t"<< DATA_PATH<<std::endl;
        std::cout << "\t Check that the path exists and modify config.h if needed"<<std::endl;
        assert(false);
    }
    else {
        infile2.close();
    }
    
    
    earth.reset(new Earth());
    
    eqd = EarthquakeDatabase(DATA_PATH);
    playbackScale = 86400;
    currentTime = eqd.getByIndex(eqd.getMinIndex()).getDate().asSeconds();
	earthquakeVisualRed.reset(new Sphere(vec3(0, 0, 0), 1, vec4(1, 0, 0, 1)));
	earthquakeVisualOrange.reset(new Sphere(vec3(0, 0, 0), 1, vec4(1, 0.5, 0, 1)));
	earthquakeVisualYellow.reset(new Sphere(vec3(0, 0, 0), 1, vec4(1, 1, 0, 1)));
    
    initializeText();
    
    rotation = mat4(1.0);
    playing = true;
}

App::~App(){
    glfonsDelete(fs);
}
    
void App::initializeText(){
    int fontNormal = FONS_INVALID;
    fs = nullptr;
    
    fs = glfonsCreate(512, 512, FONS_ZERO_TOPLEFT);
    if (fs == NULL) {
        assert(false);//Could not create stash
    }
    
    fontNormal = fonsAddFont(fs, "sans", "DroidSansMono.ttf");
    if (fontNormal == FONS_INVALID) {
        assert(false);// Could not add font normal.
    }
    
    unsigned int white = glfonsRGBA(255,255,255,255);
    
    fonsClearState(fs);
    fonsSetSize(fs, 40);
    fonsSetFont(fs, fontNormal);
    fonsSetColor(fs, white);
    fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
    
    _textShader.compileShader("textRendering.vert", GLSLShader::VERTEX);
    _textShader.compileShader("textRendering.frag", GLSLShader::FRAGMENT);
    _textShader.link();
}
    
void App::onEvent(shared_ptr<Event> event) {
    string name = event->getName();
    
	// Speedup or slowdown the playback
	if (name == "kbd_LEFT_down" || name == "kbd_LEFT_repeat") {
		playbackScale /= 1.3;
	}
	if (name == "kbd_RIGHT_down" || name == "kbd_RIGHT_repeat") {
		playbackScale *= 1.3;
	}

	// Dolly the camera closer or farther away from the earth
	if (name == "kbd_UP_down"|| name == "kbd_UP_repeat") {
		vec3 newCamPos = eye_world + vec3(0,0,-0.01);
		if (newCamPos.z > 1.2) {
			eye_world = newCamPos;
		}
	}
	else if (name == "kbd_DOWN_down" || name == "kbd_DOWN_repeat") {
		vec3 newCamPos = eye_world + vec3(0,0,0.01);
        eye_world = newCamPos;
	}

	// Rotate the earth when the user clicks and drags the mouse
	else if (name == "mouse_btn_left_down") {
        mouseDown = true;
        lastMousePos = event->get2DData();
    }
    else if (name == "mouse_btn_left_up") {
        mouseDown = false;
    }
    else if (name == "mouse_pointer"){
        if (mouseDown){
			vec2 mousePos = vec2(event->get2DData());
            vec2 dxy = mousePos - lastMousePos;

			//Sometimes the event is triggered but the diff is <0, 0>
			if (!(dxy.y == 0 && dxy.x == 0)) {
				float movementLength = glm::length(dxy) / 20000.0f;
				dxy = normalize(dxy);
				mat4 axisRotate = toMat4(angleAxis(movementLength, vec3(dxy.y, dxy.x, 0)));
				rotation = axisRotate * rotation;
			}
        }
	}
}


void App::onSimulation(double rdt) {
    // Advance the current time and loop back to the start if time is past the last earthquake
	currentTime += playbackScale * rdt;
	if (currentTime > eqd.getByIndex(eqd.getMaxIndex()).getDate().asSeconds()) {
		currentTime = eqd.getByIndex(eqd.getMinIndex()).getDate().asSeconds();
	}
	if (currentTime < eqd.getByIndex(eqd.getMinIndex()).getDate().asSeconds()) {
		currentTime = eqd.getByIndex(eqd.getMaxIndex()).getDate().asSeconds();
	}

	//Keep user from getting globe crazily rotated
	vec3 rotatedYAxis = glm::column(rotation, 1);

	//This version tries to make the y-axis perfectly straight up and down, including in the z-direction.
	//i.e. it tries to make it so you arer always staring at the equator
	/*
	float angleFromYAxis = acos(glm::dot(rotatedYAxis, vec3(0, 1, 0)));
	if (!mouseDown && angleFromYAxis > 0) {
		float angleToRotate;
		if (angleFromYAxis < radians(10.0f)) {
			angleToRotate = 0;// std::min(angleFromYAxis, radians(0.7f));
		}
		else {
			angleToRotate = (angleFromYAxis * abs(angleFromYAxis)) / 15.0f;
		}
		vec3 rotationAxis = glm::cross(rotatedYAxis, vec3(0, 1, 0));
		rotation = glm::rotate(mat4(1.0), angleToRotate, rotationAxis) * rotation;
	}
	*/

	//This version just tries to make sure the north pole is up - it doesn't care about latitude.
	if (!mouseDown) {
		
		float screenAngle = atan2(rotatedYAxis.y, rotatedYAxis.x) - radians(90.0);
		//float amntToRotate = std::min(abs(screenAngle), 0.1f);
		rotation = glm::rotate(mat4(1.0), -screenAngle / 12, vec3(0, 0, 1)) * rotation;
	}
}

void App::onRenderGraphics() {
    double curTime = glfwGetTime();
    onSimulation(curTime - lastTime);
    lastTime = curTime;


    glm::mat4 view = glm::lookAt(eye_world, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
    
    // Setup the projection matrix so that things are rendered in perspective
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), (GLfloat)_windowWidth / (GLfloat)_windowHeight, 0.1f, 500.0f);
    
    // Set a rotation matrix to apply when drawing the earth and earthquakes
    glm::mat4 model = rotation;
    
    // Update shader variables
    _shader.setUniform("view_mat", view);
    _shader.setUniform("projection_mat", projection);
    _shader.setUniform("model_mat", model);
    _shader.setUniform("eye_world", eye_world);

	// Draw the earth 
	earth->draw(_shader);

	// Draw earthquakes
	fonsSetAlign(fs, FONS_ALIGN_CENTER | FONS_ALIGN_MIDDLE);

	int start = eqd.getIndexByDate(Date(currentTime - PLAYBACK_WINDOW));
	int end = eqd.getIndexByDate(Date(currentTime));
	for (int x=start; x<end; x++) {
		Earthquake e = eqd.getByIndex(x);
		
		vec3 pt = earth->getPosition(radians(e.getLatitude()), radians(e.getLongitude()));

		float magnitude = e.getMagnitude();
		float radius = magnitude * 0.0025;

		mat4 eMatrix = model * glm::translate(mat4(1.0), pt) * glm::scale(mat4(1.0), vec3(radius, radius, radius));

		if (magnitude < 6) {
			earthquakeVisualYellow->draw(_shader, eMatrix);
		}
		else if (magnitude < 7) {
			earthquakeVisualOrange->draw(_shader, eMatrix);
		}
		else {
			earthquakeVisualRed->draw(_shader, eMatrix);
		}

		stringstream stream;
		stream << fixed << setprecision(1) << e.getMagnitude();
		string s = stream.str();

		drawString(s, radians(e.getLatitude()), radians(e.getLongitude()), view, projection);
	}
	fonsSetAlign(fs, FONS_ALIGN_LEFT | FONS_ALIGN_TOP);
    drawText();
}


void App::drawString(const std::string text, float latitude, float longitude, mat4 view, mat4 projection) {

	_textShader.use();

	vec3 pos = earth->getPosition(latitude, longitude);

	pos = rotation * vec4(pos, 1);
	pos.y -= 0.05;
	pos.z += 0.1;

	glm::mat4 model =
		// Translate so it's on the surface of the eath
		glm::translate(mat4(1.0), pos) *
		//Scale even more
		glm::scale(mat4(1.0), vec3(0.05f)) * 
		//Scale based on font text size
		glm::scale(mat4(1.0), vec3(1/40.0f)) *
		//Flip text
		glm::scale(mat4(1.0), vec3(1, -1, 1));

	_textShader.setUniform("projection_mat", projection);
	_textShader.setUniform("view_mat", view);
	_textShader.setUniform("model_mat", model);

	_textShader.setUniform("lambertian_texture", 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	fonsDrawText(fs, 0, 0, text.c_str(), NULL);

	glDisable(GL_BLEND);
	_shader.use();
}

void App::drawText() {
	Date d(currentTime);
	stringstream s;
	s << "Current date: " << d.getMonth()
		<< "/" << d.getDay()
		<< "/" << d.getYear()
		<< "  " << d.getHour()
		<< ":" << d.getMinute();
    const std::string text = s.str();
    
    _textShader.use();

    _textShader.setUniform("projection_mat", glm::ortho(0.f, (GLfloat)_windowWidth, (GLfloat)_windowHeight, 0.f, -1.f, 1.f));
    _textShader.setUniform("view_mat", glm::mat4(1.0));
    _textShader.setUniform("model_mat", glm::mat4(1.0));
    _textShader.setUniform("lambertian_texture", 0);
    
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    fonsDrawText(fs, 100, 100, text.c_str(), NULL);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    _shader.use();
}
                
}//namespace





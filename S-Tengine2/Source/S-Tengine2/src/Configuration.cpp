#pragma once

#include <Configuration.h>
#include <FileUtils.h>
#include <Log.h>
#include <GLFW\glfw3.h>
#include <ctime>

unsigned int getScaleMode(std::string _scaleMode){
	if(_scaleMode == "GL_NEAREST"){
		return GL_NEAREST;
	}else if(_scaleMode == "GL_LINEAR"){
		return GL_LINEAR;
	}else if(_scaleMode == "GL_NEAREST_MIPMAP_NEAREST"){
		return GL_NEAREST_MIPMAP_NEAREST;
	}else if(_scaleMode == "GL_LINEAR_MIPMAP_NEAREST"){
		return GL_LINEAR_MIPMAP_NEAREST;
	}else if(_scaleMode == "GL_NEAREST_MIPMAP_LINEAR"){
		return GL_NEAREST_MIPMAP_LINEAR;
	}else if(_scaleMode == "GL_LINEAR_MIPMAP_LINEAR"){
		return GL_LINEAR_MIPMAP_LINEAR;
	}else{
		throw "invalid scale mode";
	}
}

Configuration::Configuration() :
	fullscreen(false),
	resolution(0),
	fps(0),
	monitor(0),
	rngSeed(0),
	title("Untitled"),
	useLibOVR(false),
	nodeCounting(false),
	windowDecorated(true),
	windowResizable(true)
{
}

void Configuration::load(const std::string & _filename){
	// get the current video mode so that we can set the resolution defaults
	const GLFWvidmode * vidmode = glfwGetVideoMode(glfwGetPrimaryMonitor());
	bool fullscreenDefault = false;
	glm::uvec2 resolutionDefault(0);
	double fpsDefault = 60.0;
	signed long int rngSeedDefault = 0;
	signed long int monitorDefault = 0;



	Json::Reader reader;
	Json::Value json;
	bool parsingSuccessful;
	
	if(!sweet::FileUtils::fileExists(_filename)){
		// set defaults
		fullscreen = fullscreenDefault;
		resolution = resolutionDefault;
		fps = fpsDefault;

		// save defaults to file
		save(_filename);
	}
	sweet::FileUtils::createFileIfNotExists(_filename);
	
	std::string jsonLoaded = sweet::FileUtils::readFile(_filename);
	parsingSuccessful = reader.parse(jsonLoaded, json);

	if(!parsingSuccessful){
		Log::error("JSON parse failed: " + reader.getFormattedErrorMessages()/* + "\n" + jsonLoaded*/);
	}else{
		fullscreen = json.get("fullscreen", fullscreenDefault).asBool();
		resolution = glm::uvec2(json["resolution"].get("x", resolutionDefault.x).asInt(), json["resolution"].get("y", resolutionDefault.y).asInt());
		fps = json.get("fps", fpsDefault).asDouble();
		rngSeed = json.get("rngSeed", rngSeedDefault).asInt();
		monitor = json.get("monitor", monitorDefault).asInt();
		useLibOVR = json.get("useLibOVR", false).asBool();
		nodeCounting = json.get("nodeCounting", false).asBool();
		
		generateMipmapsDefault = json.get("generateMipmapsDefault", true).asBool();
		scaleModeMagDefault = getScaleMode(json.get("scaleModeMagDefault", "GL_LINEAR").asString());
		scaleModeMinDefault = getScaleMode(json.get("scaleModeMinDefault", "GL_LINEAR_MIPMAP_LINEAR").asString());
		
		windowDecorated = json.get("windowDecorated", true).asBool();
		windowResizable = json.get("windowResizable", true).asBool();

		title = json.get("title", "Untitled").asString();
		Json::Value jsonGain = json["gain"];

		// gain defaults
		gain["master"] = gain["voice"] = gain["music"] = gain["sfx"] = gain["other"] = 1;

		for(auto g : jsonGain.getMemberNames()){
			gain[g] = jsonGain.get(g, 1.f).asFloat();
		}

		// if the rng seed is negative, it means we want to use the time as a seed
		if(rngSeed < 0){
			rngSeed = time(nullptr);
		}
	}
}

void Configuration::save(const std::string & _filename){
	Json::Value json;
	json["fullscreen"] = fullscreen;
	json["resolution"]["x"] = resolution.x;
	json["resolution"]["y"] = resolution.y;
	json["fps"] = fps;
	json["rngSeed"] = rngSeed;
	json["monitor"] = monitor;
	json["useLibOVR"] = useLibOVR;
	json["nodeCounting"] = nodeCounting;
	json["title"] = title;
	json["gain"] = Json::Value();
	for(auto g : gain){
		json["gain"][g.first] = g.second;
	}

	std::ofstream log(_filename, std::ios_base::out);
	log << json;
	log.close();
}
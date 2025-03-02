#pragma once

#include <Scene_Splash.h>

#include <shader/ShaderComponentMVP.h>
#include <shader/ShaderComponentTexture.h>

#include <MeshInterface.h>
#include <MeshFactory.h>
#include <Game.h>
#include <RenderOptions.h>
#include <scenario/Asset.h>

#include <sweet/Input.h>

Scene_Splash::Scene_Splash(Game * _game, Texture * _splashImage, OpenAL_Sound * _splashSound, float _length, glm::vec3 _clearColour) :
	Scene(_game),
	clearColour(_clearColour),
	splash(nullptr),
	shader(new ComponentShaderBase(true)),
	alphaComponent(new ShaderComponentAlpha(shader, 0)),
	nextScene(""),
	aspectRatio(1.f),
	splashSound(_splashSound),
	splashImage(_splashImage)
{
	shader->addComponent(new ShaderComponentMVP(shader));
	shader->addComponent(new ShaderComponentTexture(shader));
	shader->addComponent(alphaComponent);
	shader->compileShader();
	shader->nodeName = "Splash Scene base shader";

	// add orthographic camera
	orthoCam = new OrthographicCamera(0, 0, 0, 0, -1000, 1000);
	childTransform->addChild(orthoCam);
	cameras.push_back(orthoCam);
	activeCamera = orthoCam;
	orthoCam->nodeName = "Splash Scene ortho cam";
	
	// add splash image MeshEntity
	splash = new MeshEntity(MeshFactory::getPlaneMesh(), shader);
	childTransform->addChild(splash);
	splash->mesh->pushTexture2D(_splashImage);

	// initialize timer
	timer = new Timeout(_length, [this](sweet::Event * _event){
		game->switchScene(nextScene, true);
	});

	// add a default fade-in/out transition
	timer->eventManager->addEventListener("progress", [this](sweet::Event * _event){
		float p = _event->getFloatData("progress");
		if(p <= 0.1f){
			alphaComponent->setAlpha(p*10.f);
		}else if(p >= 0.9f){
			alphaComponent->setAlpha((1.f - p)*10.f);
		}else{
			alphaComponent->setAlpha(1.f);
		}
	});

	// when the timer starts, load the image and play the sound (if provided)
	timer->eventManager->addEventListener("start", [this](sweet::Event * _event){
		splashImage->load();
		aspectRatio = (float)splashImage->width/splashImage->height;
		if(splashSound != nullptr){
			splashSound->play();
		}
	});
	timer->start();

	childTransform->addChild(timer, false);

	splashSound->incrementReferenceCount();
	splashImage->incrementReferenceCount();
	shader->incrementReferenceCount();
}

Scene_Splash::~Scene_Splash(){
	splashSound->decrementAndDelete();
	splashImage->decrementAndDelete();
	shader->decrementAndDelete();
}

void Scene_Splash::update(Step * _step){
	glm::uvec2 sd = sweet::getWindowDimensions();

	orthoCam->resize(0, sd.x, 0, sd.y);

	// center and resize the splash image
	if(splash != nullptr){
		float sx;
		float sy;
		// screen aspect ratio
		float sar = (float)sd.x/sd.y;
		if(sar > 1){
			if(aspectRatio > sar){
				sx = sd.x;
				sy = sd.x/aspectRatio;
			}else{
				sx = sd.y*aspectRatio;
				sy = sd.y;
			}
		}else{
			if(aspectRatio > sar){
				sx = sd.x;
				sy = sd.x/aspectRatio;
			}else{
				sx = sd.y*aspectRatio;
				sy = sd.y;
			}
		}
		splash->childTransform->scale(sx, sy, 1, false);
		splash->childTransform->translate(sd.x/2.f,sd.y/2.f,0,false);
	}

	if(Keyboard::getInstance().keyJustUp(GLFW_KEY_ENTER) || Mouse::getInstance().leftJustReleased()){
		splashSound->stop();
		timer->eventManager->triggerEvent("complete");
	}
	
	Scene::update(_step);
}

void Scene_Splash::render(sweet::MatrixStack * _matrixStack, RenderOptions * _renderOptions){
	// don't bother doing any work if we aren't rendering anyway
	if(!isVisible()){
		return;
	}
	_renderOptions->setClearColour(clearColour.r, clearColour.g, clearColour.b, 1.f);
	_renderOptions->clear();
	Scene::render(_matrixStack, _renderOptions);
}

void Scene_Splash::load(){
	if(!loaded){
		shader->load();
	}
	Scene::load();	
}

void Scene_Splash::unload(){
	if(loaded){
		shader->unload();
	}
	Scene::unload();
}
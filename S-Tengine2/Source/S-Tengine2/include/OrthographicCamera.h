#pragma once

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Camera.h"

class Mouse;

/****************************
*
* An orthographic camera class. This camera represents 3d objects in a 2d view.
*
*****************************/
class OrthographicCamera : public Camera {
public:

	OrthographicCamera(float _left, float _right, float _bottom, float _top, float _nearClip, float _farClip);

	/**
	* @return The projection matrix of the camera 
	*/
	glm::mat4 getProjectionMatrix(glm::vec2 _screenSize) const override;
	
	float left;
	float top;
	float bottom;
	float right;

	void resize(float _left, float _right, float _bottom, float _top);
	
	// returns right - left
	float getWidth() const;
	// returns top - bottom
	float getHeight() const;
};
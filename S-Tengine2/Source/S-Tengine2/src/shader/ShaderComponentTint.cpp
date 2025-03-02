#pragma once

#include <shader/ShaderComponentTint.h>
#include <shader/ShaderVariables.h>
#include <shader/ComponentShaderBase.h>
#include <GLUtils.h>
#include <RenderOptions.h>

ShaderComponentTint::ShaderComponentTint(ComponentShaderBase * _shader, float _r, float _g, float _b):
	ShaderComponent(_shader),
	red(_r),
	green(_g),
	blue(_b),
	redLoc(-1),
	greenLoc(-1),
	blueLoc(-1)
{
}

ShaderComponentTint::~ShaderComponentTint(){
}
	
std::string ShaderComponentTint::getVertexVariablesString(){
	return DEFINE + SHADER_COMPONENT_TINT + ENDL;
}

std::string ShaderComponentTint::getFragmentVariablesString(){
	return 
		DEFINE + SHADER_COMPONENT_TINT + ENDL +
		"uniform float " + GL_UNIFORM_ID_TINT_RED + SEMI_ENDL + 
		"uniform float " + GL_UNIFORM_ID_TINT_GREEN + SEMI_ENDL +
		"uniform float " + GL_UNIFORM_ID_TINT_BLUE + SEMI_ENDL;
}

std::string ShaderComponentTint::getVertexBodyString(){
	return "";
}

std::string ShaderComponentTint::getFragmentBodyString(){
	return "";
}

std::string ShaderComponentTint::getOutColorMod(){
	return
		"// modify rgb" + ENDL +
		GL_OUT_OUT_COLOR + ".r += " + GL_UNIFORM_ID_TINT_RED + SEMI_ENDL +
		GL_OUT_OUT_COLOR + ".g += " + GL_UNIFORM_ID_TINT_GREEN + SEMI_ENDL +
		GL_OUT_OUT_COLOR + ".b += " + GL_UNIFORM_ID_TINT_BLUE + SEMI_ENDL;
}

void ShaderComponentTint::load(){
	if(!loaded){
		redLoc = glGetUniformLocation(shader->getProgramId(), GL_UNIFORM_ID_TINT_RED.c_str());
		greenLoc = glGetUniformLocation(shader->getProgramId(), GL_UNIFORM_ID_TINT_GREEN.c_str());
		blueLoc = glGetUniformLocation(shader->getProgramId(), GL_UNIFORM_ID_TINT_BLUE.c_str());
	}
	ShaderComponent::load();
}

void ShaderComponentTint::configureUniforms(sweet::MatrixStack * _matrixStack, RenderOptions * _renderOption, NodeRenderable * _nodeRenderable){
	glUniform1f(redLoc, red);
	glUniform1f(greenLoc, green);
	glUniform1f(blueLoc, blue);
}

void ShaderComponentTint::setRed(float _r){
	red = _r;
	makeDirty();
}
void ShaderComponentTint::setGreen(float _g){
	green = _g;
	makeDirty();
}
void ShaderComponentTint::setBlue(float _b){
	blue = _b;
	makeDirty();
}
void ShaderComponentTint::setRGB(float _r, float _g, float _b){
	red = _r;
	green = _g;
	blue = _b;
	makeDirty();
}

float ShaderComponentTint::getRed(){
	return red;
}
float ShaderComponentTint::getGreen(){
	return green;
}
float ShaderComponentTint::getBlue(){
	return blue;
}
glm::vec3 ShaderComponentTint::getRGB(){
	return glm::vec3(red, green, blue);
}
#pragma once

#include <shader/ComponentShaderBase.h>
#include "shader/ShaderComponentText.h"
#include "shader/ShaderVariables.h"
#include "MatrixStack.h"
#include "RenderOptions.h"
#include "Texture.h"
#include "node/NodeRenderable.h"
#include "MeshInterface.h"
#include "Sprite.h"
#include "SpriteSheetAnimation.h"

ShaderComponentText::ShaderComponentText(ComponentShaderBase * _shader) :
	ShaderComponent(_shader),
	texNumLoc(-1),
	texColLoc(-1),
	texSamLoc(-1),
	numTextures(0),
	color(0, 0, 0, 1)
{
}

ShaderComponentText::~ShaderComponentText(){
}

std::string ShaderComponentText::getVertexVariablesString(){
	return DEFINE + SHADER_COMPONENT_TEXT + ENDL;
}

std::string ShaderComponentText::getFragmentVariablesString(){
	return 
		DEFINE + SHADER_COMPONENT_TEXT + ENDL + 
		IF_NOT_DEFINED + SHADER_COMPONENT_TEXTURE + ENDL + 
		"uniform sampler2D " + GL_UNIFORM_ID_TEXTURE_SAMPLER + "[" + std::to_string(MAX_TEXTURES) + "]" + SEMI_ENDL + 
		"uniform int " + GL_UNIFORM_ID_NUM_TEXTURES + SEMI_ENDL + 
		END_IF + ENDL +
		"uniform vec4 " + GL_UNIFORM_ID_TEXT_COLOR + SEMI_ENDL;
}

std::string ShaderComponentText::getVertexBodyString(){
	return "";
}

std::string ShaderComponentText::getFragmentBodyString(){
    std::stringstream res;
	res << "vec4 textModFrag = vec4(0, 0, 0, 0)" << SEMI_ENDL;
    res << "if(" << GL_UNIFORM_ID_NUM_TEXTURES << " > 0){" << ENDL;
		res << "textModFrag = vec4(" << GL_UNIFORM_ID_TEXT_COLOR << ".rgb, texture(" << GL_UNIFORM_ID_TEXTURE_SAMPLER << "[" << 0 << "], " << GL_IN_OUT_FRAG_UV << ").r * " << GL_UNIFORM_ID_TEXT_COLOR << ".a)" << SEMI_ENDL;
    res << "}" << ENDL;
    return res.str();
}

std::string ShaderComponentText::getOutColorMod(){
	return GL_OUT_OUT_COLOR + " *= textModFrag" + SEMI_ENDL;
}

glm::vec4 ShaderComponentText::getColor() const{
	return color;
}

void ShaderComponentText::setColor(glm::vec4 _color){
	color = _color;
	makeDirty();
}

void ShaderComponentText::clean(sweet::MatrixStack* _matrixStack, RenderOptions* _renderOption, NodeRenderable* _nodeRenderable){
	//ShaderComponent::clean(_matrixStack, _renderOption, _nodeRenderable);
	configureUniforms(_matrixStack, _renderOption, _nodeRenderable);
	shader->configureUniforms(_matrixStack, _renderOption, _nodeRenderable);
}

void ShaderComponentText::load(){
	if(!loaded){
		texNumLoc = glGetUniformLocation(shader->getProgramId(), GL_UNIFORM_ID_NUM_TEXTURES.c_str());
		texColLoc = glGetUniformLocation(shader->getProgramId(), GL_UNIFORM_ID_TEXT_COLOR.c_str());
		texSamLoc = glGetUniformLocation(shader->getProgramId(), GL_UNIFORM_ID_TEXTURE_SAMPLER.c_str());
	}
	ShaderComponent::load();
}

void ShaderComponentText::unload(){
	if(loaded){
		texNumLoc = -1;
		texColLoc = -1;
		texSamLoc = -1;
		numTextures = 0;
	}
	ShaderComponent::unload();
}

void ShaderComponentText::configureUniforms(sweet::MatrixStack* _matrixStack, RenderOptions* _renderOption, NodeRenderable* _nodeRenderable){
		texNumLoc = glGetUniformLocation(shader->getProgramId(), GL_UNIFORM_ID_NUM_TEXTURES.c_str());
		texColLoc = glGetUniformLocation(shader->getProgramId(), GL_UNIFORM_ID_TEXT_COLOR.c_str());
		texSamLoc = glGetUniformLocation(shader->getProgramId(), GL_UNIFORM_ID_TEXTURE_SAMPLER.c_str());
	MeshInterface * mesh = dynamic_cast<MeshInterface *>(_nodeRenderable);
	if(mesh != nullptr){
		// check if the number of textures has changes and send new value to OpenGL
		unsigned long int newNumTextures = mesh->textureCount();
		if(newNumTextures != numTextures){
			glUniform1i(texNumLoc, newNumTextures);
			checkForGlError(false);
			numTextures = newNumTextures;
		}
		// Bind each texture to the texture sampler array in the frag _shader
		for(unsigned long int i = 0; i < mesh->textureCount(); i++){
			glActiveTexture(GL_TEXTURE0 + i);
			glBindTexture(GL_TEXTURE_2D, mesh->getTexture(i)->textureId);
			glUniform1i(texSamLoc, i);
		}
	}else{
		glUniform1i(texNumLoc, numTextures);
	}
	glUniform4f(texColLoc, color.x, color.y, color.z, color.w);
}
#pragma once

#include <stb/stb_image.h>
#include <stb/stb_image_write.h>

#include <Texture.h>
#include <Resource.h>
#include <GLUtils.h>
#include <Log.h>

Texture::Texture(std::string _src, bool _storeData, bool _autoRelease, bool _useMipmaps) :
	NodeResource(_autoRelease),
	src(_src),
	width(0),
	height(0),
	channels(0),
	numPixels(0),
	numBytes(0),
	storeData(_storeData),
	data(nullptr),
	useMipmaps(_useMipmaps)
{
	nodeName = "Texture " + src;
}

Texture::~Texture(){
	unload();
	unloadImageData();
}

void Texture::resize(int _width, int _height, int _channels){
	width = _width;
	height = _height;
	channels = _channels;
	numPixels = width * height;
	numBytes = numPixels * channels;
}

void Texture::load(){
	if(!loaded){
		// load texture data if necessary
		if(data == nullptr){
			loadImageData();
		}
		
		genTextures();

		// delete texture data if necessary
		if(!storeData){
			unloadImageData();
		}
	}
	
	NodeLoadable::load();
}

void Texture::unload(){
	if(loaded){
		ST_LOG_INFO("Unloading texture: " + this->src);
		glDeleteTextures(1, &textureId);
		textureId = 0;
		checkForGlError(false);
	}
	NodeLoadable::unload();
}

void Texture::loadImageData(){
	int w, h, c;
	data = stbi_load(src.c_str(), &w, &h, &c, 0);
	assert(data != nullptr);
	resize(w,h,c);
}

void Texture::unloadImageData(){
	free(data);
	data = nullptr;
}

void Texture::saveImageData(const std::string & _filename, bool _reverse, bool _png){
	std::stringstream ss;
	ss << "data/images/" << _filename;

	if(_reverse){
		auto tempData = data;
		data = static_cast<unsigned char *>(malloc(sizeof(unsigned char) * numBytes));
		memcpy(data, tempData, numBytes);
		// flip the texture
		for(signed long int y = 0; y < width; ++y){
			memcpy(&tempData[y*width*channels], &data[(height-y-1)*width*channels], width*channels);
		}
		data = tempData;
	}

	if(_png){
		if(stbi_write_png(ss.str().c_str(), width, height, channels, data, channels * width)){
			Log::info("Texture \"data/images/"+_filename+"\" saved");
		}else{
			Log::error("Texture \"data/images/"+_filename+"\" not saved");
		}
	}else{
		if(stbi_write_tga(ss.str().c_str(), width, height, channels, data)){
			Log::info("Texture \"data/images/"+_filename+"\" saved");
		}else{
			Log::error("Texture \"data/images/"+_filename+"\" not saved");
		}
	}
}

void Texture::bufferData() const {
	glBindTexture(GL_TEXTURE_2D, textureId);
	checkForGlError(false);
	GLenum format;
	switch(channels){
		case 1: format = GL_RED; break;
		case 2: format = GL_RG; break;
		case 3: format = GL_RGB; break;
		case 4: format = GL_RGBA; break;
		default: throw "Invalid number of image channels";
	}
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height, format, GL_UNSIGNED_BYTE, data);
	checkForGlError(false);
	
	if (useMipmaps){
		glGenerateMipmap(GL_TEXTURE_2D);
		checkForGlError(false);
	}
}

void Texture::bufferDataFirst() const {
	glBindTexture(GL_TEXTURE_2D, textureId);
	checkForGlError(false);
	GLenum format;
	switch(channels){
		case 1: format = GL_RED; break;
		case 2: format = GL_RG; break;
		case 3: format = GL_RGB; break;
		case 4: format = GL_RGBA; break;
		default: throw "Invalid number of image channels";
	}
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	checkForGlError(false);
	
	if(useMipmaps){
		glGenerateMipmap(GL_TEXTURE_2D);
		checkForGlError(false);
	}
}


void Texture::genTextures() {
	// create and buffer OpenGL texture
	glGenTextures(1, &textureId);
	checkForGlError(false);
	bufferDataFirst();
}

﻿#pragma once

#include <Font.h>
#include <Sweet.h>
#include <MeshFactory.h>

GlyphMesh::GlyphMesh(FT_GlyphSlot _glyph, bool _antiAliased) :
	QuadMesh(false),
	NodeResource(false)
{
	// get a standard plane mesh and insert the verts into the glyph
	MeshInterface * m = MeshFactory::getPlaneMesh(0.5f, false);
	insertVertices(*m);
	delete m;
	set(_glyph, _antiAliased);
	uvEdgeMode = GL_CLAMP;
}

void GlyphMesh::set(FT_GlyphSlot _glyph, bool _antiAliased){
	// get the glyph dimensions
	float vx = _glyph->bitmap_left;
	float vy = _glyph->bitmap_top;
	float w = _glyph->bitmap.width;
	float h = _glyph->bitmap.rows;
	
	// modify the mesh verts to match the glyph dimensions
	vertices.at(0).x = vx;
	vertices.at(0).y = vy;
	
	vertices.at(1).x = vx + w;
	vertices.at(1).y = vy;
	
	vertices.at(2).x = vx + w;
	vertices.at(2).y = vy - h;
	
	vertices.at(3).x = vx;
	vertices.at(3).y = vy - h;
	dirty = true;

	// set the scale mode
	if(!_antiAliased){
		setScaleMode(GL_NEAREST);
	}else{
		setScaleMode(GL_LINEAR);
	}
}



GlyphTexture::GlyphTexture(FT_Bitmap _glyph) :
	Texture("", true, false),
	NodeResource(false)
{
	set(_glyph);
}

void GlyphTexture::set(FT_Bitmap _glyph){
	unload();

	width = _glyph.width;
	height = _glyph.rows;
	data = new unsigned char[_glyph.rows*_glyph.width];
	memcpy(data, _glyph.buffer, _glyph.rows*_glyph.width);
	//data = _glyph.buffer;
	channels = 2;

	load();
}

void GlyphTexture::load(){
	if(!loaded){
		// The textures from FreeType are single channel and use a different pixel store alignment
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glGenTextures(1, &textureId);
		checkForGlError(false);
		glBindTexture(GL_TEXTURE_2D, textureId);
		checkForGlError(false);

		glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, width, height, 0, GL_RED, GL_UNSIGNED_BYTE, data);
		
		glGenerateMipmap(GL_TEXTURE_2D);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	}
	
	NodeLoadable::load();
}

Glyph::Glyph(FT_GlyphSlot _glyph, wchar_t _character, bool _antialiased) :
	NodeResource(false),
	character(_character),
	texture(nullptr),
	mesh(nullptr)
{
	setGlyph(_glyph, _character, _antialiased);
}

Glyph::~Glyph(){
	delete mesh;
	delete texture;
}

void Glyph::load(){
	if(!loaded){
		texture->load();
		mesh->load();
		NodeResource::load();
	}
}
void Glyph::unload(){
	if(loaded){
		texture->unload();
		mesh->unload();
		NodeResource::unload();
	}
}
void Glyph::setGlyph(FT_GlyphSlot _glyph, wchar_t _character, bool _antiAliased){
	advance = _glyph->advance;
	metrics = _glyph->metrics;
	bitmap_left = _glyph->bitmap_left;
	bitmap_top = _glyph->bitmap_top;

	if(texture == nullptr){
		texture = new GlyphTexture(_glyph->bitmap);
	}else{
		texture->set(_glyph->bitmap);
	}
	if(mesh == nullptr){
		mesh = new GlyphMesh(_glyph, _antiAliased);
		mesh->pushTexture2D(texture);
	}else{
		mesh->set(_glyph, _antiAliased);
	}
}


Font::Font(std::string _fontSrc, float _size, bool _autoRelease, FontScaleMode _scaleMode) :
	NodeResource(_autoRelease),
	face(nullptr),
	antiAliased(false),
	size(_size),
	scaleMode(_scaleMode)
{
	if(FT_New_Face(sweet::freeTypeLibrary, _fontSrc.c_str(), 0, &face) != 0) {
		Log::error("Couldn't load font: " + _fontSrc);
	}
	lineGapRatio = 1.f;
}

Font::~Font(){
	for(auto g : glyphs){
		delete g.second;
	}
	FT_Done_Face(face);
}

void Font::load(){
	if(!loaded){
		for(auto g : glyphs){
			g.second->load();
		}
	}
	NodeResource::load();
}

void Font::unload(){
	if(loaded){
		for(auto g : glyphs){
			g.second->unload();
		}
	}
	NodeResource::unload();
}

Glyph * Font::getGlyphForChar(wchar_t _char){
	Glyph * res;
	auto g = glyphs.find(_char);
	if(g != glyphs.end()){
		res = g->second;
	}else{
		loadGlyph(_char);
		res = new Glyph(face->glyph, _char, antiAliased);
		glyphs[_char] = res;
	}
	
	res->load();
	return res;
}

GlyphTexture * Font::getTextureForChar(wchar_t _char){
	return getGlyphForChar(_char)->texture;
}

GlyphMesh * Font::getMeshInterfaceForChar(wchar_t _char){
	return getGlyphForChar(_char)->mesh;
}

glm::vec2 Font::getGlyphWidthHeight(wchar_t _char){
	GlyphTexture * t = getGlyphForChar(_char)->texture;
	return glm::vec2(t->width, t->height);
}

glm::vec2 Font::getGlyphAdvance(wchar_t _char){
	Glyph * glyph = getGlyphForChar(_char);
	return glm::vec2(glyph->advance.x/64.f, glyph->advance.y/64.f);
}

glm::vec2 Font::getGlyphXY(wchar_t _char){
	Glyph * g = getGlyphForChar(_char);
	return glm::vec2(g->bitmap_left, g->bitmap_top);
}

void Font::loadGlyph(wchar_t _char) const {
	if(scaleMode == FontScaleMode::kPOINT){
		float dpi = sweet::getDpi();
		FT_Set_Char_Size(
			  face,    /* handle to face object           */
			  0,       /* char_width in 1/64th of points -- 0 = same as height */
			  size * 64,   /* char_height in 1/64th of points */
			  dpi,     /* horizontal device resolution    */
			  dpi);   /* vertical device resolution      */
	}else {
		float sizeCalc = size;
		if(scaleMode == FontScaleMode::kPERCENT) {
			sizeCalc = sweet::getWindowHeight() * (size * 0.01f);
		}
		FT_Set_Pixel_Sizes(
			face,
			0,
			sizeCalc);
	}
	FT_Load_Char(face, _char, FT_LOAD_RENDER);
}

float Font::getLineHeight() const {
	loadGlyph(L'X');
	return lineGapRatio * face->size->metrics.height/64.f;
}


void Font::resize(float _size){
	size = _size;
	for(auto g : glyphs){
		loadGlyph(g.first);
		g.second->setGlyph(face->glyph, g.first, antiAliased);
	}
}

float Font::getSize(){
	return size;
}
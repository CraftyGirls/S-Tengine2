#pragma once

#include <MeshFactory.h>
#include <NodeUI.h>
#include <Scene.h>
#include <Camera.h>
#include <Mouse.h>
#include <MeshInterface.h>
#include <FrameBufferInterface.h>
#include <StandardFrameBuffer.h>
#include <Texture.h>
#include <FBOTexture.h>

#include <shader\ComponentShaderBase.h>
#include <shader\ShaderComponentTexture.h>
#include <shader\ShaderComponentAlpha.h>
#include <shader\ShaderComponentTint.h>
#include <shader\ShaderComponentMVP.h>

#include <NumberUtils.h>
#include <MatrixStack.h>
#include <RenderOptions.h>
#include <Texture.h>
#include <OrthographicCamera.h>
#include <StandardFrameBuffer.h>
#include <algorithm>
#include <Layout.h>
#include <shader/ShaderComponentDepthOffset.h>

ComponentShaderBase  * NodeUI::bgShader = nullptr;
ShaderComponentTint  * NodeUI::bgTintComponent = nullptr;
ShaderComponentAlpha * NodeUI::bgAlphaComponent = nullptr;

NodeUI::NodeUI(BulletWorld * _world, RenderMode _renderMode, bool _mouseEnabled) :
	NodeBulletBody(_world),
	eventManager(new sweet::EventManager()),
	mouseEnabled(!_mouseEnabled),
	mouse(&Mouse::getInstance()),
	isHovered(false),
	isDown(false),
	isActive(false),
	uiElements(new Transform()),
	bgColour(0.f, 0.f, 0.f, 1.f),
	nodeUIParent(nullptr),
	background(new Plane(glm::vec3(-0.5f, -0.5f, 0.f), 1.f, bgShader)),
	textureCam(nullptr),
	frameBuffer(nullptr),
	renderedTexture(nullptr),
	texturedPlane(nullptr),
	boxSizing(kBORDER_BOX), // we initialize the variable to the opposite so that setMouseEnabled gets called properly at the end of the constructor
	updateState(false),
	margin(new Transform()),
	padding(new Transform()),
	textureModeAlpha(1.f),
	__renderFrameDirty(_renderMode == kTEXTURE),
	__layoutDirty(true),
	renderMode(_renderMode)
{
	nodeType |= NodeType::kNODE_UI;
	ptrNodeUI = this;

	if(bgShader == nullptr){
		bgShader = new ComponentShaderBase(true);
		bgShader->incrementReferenceCount();
		bgTintComponent = new ShaderComponentTint(bgShader);
		bgAlphaComponent = new ShaderComponentAlpha(bgShader);
		bgShader->addComponent(new ShaderComponentMVP(bgShader));
		bgShader->addComponent(new ShaderComponentTexture(bgShader));
		bgShader->addComponent(bgTintComponent);
		bgShader->addComponent(bgAlphaComponent);
		bgShader->addComponent(new ShaderComponentDepthOffset(bgShader));
		bgShader->compileShader();
		bgShader->nodeName = "NodeUI background shader";
		background->setShader(bgShader, true);
	}

	childTransform->addChild(margin, false);
	margin->addChild(background, true);
	margin->addChild(padding, false);
	padding->addChild(uiElements, false);

	paddingLeft.setPixelSize(0);
	paddingRight.setPixelSize(0);
	paddingTop.setPixelSize(0);
	paddingBottom.setPixelSize(0);
	marginLeft.setPixelSize(0);
	marginRight.setPixelSize(0);
	marginBottom.setPixelSize(0);
	marginTop.setPixelSize(0);

	setMouseEnabled(_mouseEnabled);
}

NodeUI::~NodeUI() {
	if(textureCam != nullptr){
		delete textureCam;
		delete frameBuffer;
		delete texturedPlane;
	}
	delete eventManager;
}

void NodeUI::setVisible(bool _visible){
	if(_visible != isVisible()){
		invalidateRenderFrame();
		NodeBulletBody::setVisible(_visible);
	}
}

void NodeUI::load(){
	if(!loaded){
		bgShader->load();
	}
	Entity::load();
}
void NodeUI::unload(){
	if(loaded){
		bgShader->unload();

		if(textureCam != nullptr){
			delete textureCam;
			delete frameBuffer;
			delete texturedPlane;
		}

		invalidateRenderFrame();
	}
	Entity::unload();
}

void NodeUI::down(){
	isHovered = true;
	isDown = true;

	triggerEvent("mousedown");
}
void NodeUI::up(){
	if(isHovered && isDown){
		click();
	}
	isDown = false;

	triggerEvent("mouseup");
}
void NodeUI::click(){
	triggerEvent("click");
}
void NodeUI::in(){
	isHovered = true;

	triggerEvent("mousein");
}
void NodeUI::out(){
	isHovered = false;

	triggerEvent("mouseout");
}
void NodeUI::triggerEvent(std::string _tag){
	sweet::Event * e = new sweet::Event(_tag);
	e->setIntData("target", (int)this);
	eventManager->triggerEvent(e);
}

Transform * NodeUI::addChild(NodeUI* _uiElement, bool _invalidateLayout){
	_uiElement->nodeUIParent = this;
	if(_invalidateLayout){
		invalidateLayout();
	}
	return uiElements->addChild(_uiElement);
}

signed long int NodeUI::removeChild(NodeUI* _uiElement, bool _invalidateLayout){
	unsigned long int res = -1;
	if(_uiElement->parents.size() > 0){
		Transform * t = _uiElement->firstParent();
		res = uiElements->removeChild(t);

		// if the element is a child, remove it
		if(res != (unsigned long int)-1){
			t->removeChild(_uiElement);
			delete t;
			if(_invalidateLayout){
				invalidateLayout();
			}
			_uiElement->nodeUIParent = nullptr;
		}
	}
	return res;
}

void NodeUI::translatePhysical(glm::vec3 _translation, bool _relative){
	firstParent()->translate(_translation, _relative);
}

void NodeUI::doRecursivelyOnUIChildren(std::function<void(NodeUI * _childOrThis)> _todo, bool _includeSelf) {
	for(NodeChild * c : uiElements->children) {
		Transform * t = c->asTransform();
		if(t != nullptr){
			NodeUI * nodeUI = t->children.at(0)->asNodeUI();
			if(nodeUI != nullptr) {
				nodeUI->doRecursivelyOnUIChildren(_todo, true);
			}
		}
	}
	if(_includeSelf) {
		_todo(this);
	}
}

bool NodeUI::isFirstParentNodeUI() {
	return firstParent() != nullptr && static_cast<Node *>(firstParent())->asNodeUI() != nullptr;
}

bool NodeUI::isMouseEnabled(){
	return mouseEnabled;
}

void NodeUI::setMouseEnabled(bool _mouseEnabled){
	// if nothing has changed, return early
	if(_mouseEnabled == mouseEnabled){
		return;
	}
	if(_mouseEnabled){
		// create the NodeBulletBody collider stuff
		autoResize();
		updateCollider();
	}else{
		// delete the NodeBulletBody collider stuff
		if(shape != nullptr){
			delete shape;
			shape = nullptr;
		}if(body != nullptr){
			world->world->removeRigidBody(body);
			body = nullptr;
		}
	}
	mouseEnabled = _mouseEnabled;
}

void NodeUI::update(Step * _step){
	__updateForEntities(_step);
	if(renderMode == kTEXTURE){
		__updateForTexture(_step);
	}
}

Texture * NodeUI::renderToTexture(RenderOptions * _ro){
	float h = getHeight(true, true);
	float w = getWidth(true, true);

	// if the texture is too small, just return the previously rendered one
	if(h < 1 || w < 1){
		return renderedTexture;
	}

	if(textureCam == nullptr){
		textureCam = new OrthographicCamera(0, w, 0, h, -1,1);
		textureCam->nodeName = "NodeUI texture cam";
	}

	if(frameBuffer == nullptr){
		frameBuffer = new StandardFrameBuffer(false);
		frameBuffer->load();
		frameBuffer->nodeName = "NodeUI texture framebuffer";
	}

	auto vd = _ro->viewPortDimensions;
	_ro->setViewPort(0, 0, w, h);

	texturedPlane->childTransform->translate(w * 0.5f, h * 0.5f, 0.f, false);
	texturedPlane->childTransform->scale(w, -h, 1.f, false);

	if(frameBuffer->resize(w, h) && renderedTexture != nullptr){
		textureCam->resize(0, w, 0, h);
		texturedPlane->mesh->removeTextureAt(0);
		renderedTexture = nullptr;
	}

	FrameBufferInterface::pushFbo(frameBuffer);

	GLboolean depth = glIsEnabled(GL_DEPTH_TEST);

	if(depth == GL_TRUE){
		glDisable(GL_DEPTH_TEST);
	}

	RenderOptions renderOptions(nullptr, nullptr);
	sweet::MatrixStack matrixStack;
	matrixStack.setCamera(textureCam, glm::vec2(w, h));

	renderOptions.clearColour[3] = 0; // clear to alpha, not black
	renderOptions.clear();

	__renderForEntities(&matrixStack, &renderOptions);

	if(depth == GL_TRUE){
		glEnable(GL_DEPTH_TEST);
	}

	FrameBufferInterface::popFbo();

	if(renderedTexture == nullptr) {
		renderedTexture = new FBOTexture(frameBuffer, true, 0, true, false);
		renderedTexture->load();
		renderedTexture->refresh();
		texturedPlane->mesh->pushTexture2D(renderedTexture);
	}else{
		renderedTexture->width  = w;
		renderedTexture->height = h;
		renderedTexture->numPixels = renderedTexture->width * renderedTexture->height;
		renderedTexture->numBytes = renderedTexture->numPixels * renderedTexture->channels;
		free(renderedTexture->data);
		renderedTexture->data = frameBuffer->getPixelData(0);
		renderedTexture->bufferData();
	}

	_ro->setViewPort(vd.x, vd.y, vd.width, vd.height);
	//renderedTexture->saveImageData("test.tga");
	return renderedTexture;
}

MeshEntity * NodeUI::getTexturedPlane() {
	if(texturedPlane == nullptr) {
		texturedPlane = new MeshEntity(MeshFactory::getPlaneMesh(0.5f), background->shader);
		texturedPlane->mesh->setScaleMode(GL_NEAREST);
	}
	return texturedPlane;
}

void NodeUI::render(sweet::MatrixStack * _matrixStack, RenderOptions * _renderOptions){
	// don't bother doing any work if we aren't rendering anyway
	if(!isVisible()){
		__renderFrameDirty = false;
		return;
	}

	if(renderMode == kENTITIES){
		__renderForEntities(_matrixStack, _renderOptions);
	}

	if(renderMode == kTEXTURE){
		__renderForTexture(_matrixStack, _renderOptions);
	}
}

void NodeUI::__renderForEntities(sweet::MatrixStack * _matrixStack, RenderOptions * _renderOptions) {
	bgTintComponent->setRGB(bgColour.r, bgColour.g, bgColour.b);
	bgAlphaComponent->setAlpha(bgColour.a);
	Entity::render(_matrixStack, _renderOptions);
	__renderFrameDirty = false;
}

void NodeUI::__renderForTexture(sweet::MatrixStack * _matrixStack, RenderOptions * _renderOptions) {
	getTexturedPlane();
	if(isRenderFrameDirty() && active) {
		renderToTexture(_renderOptions);
	}

	bgTintComponent->setRGB(0, 0, 0);
	bgAlphaComponent->setAlpha(textureModeAlpha);

	texturedPlane->render(_matrixStack, _renderOptions);
	__renderFrameDirty = false;
}

void NodeUI::__updateForEntities(Step * _step) {
	if(active){
		eventManager->update(_step);
		if(isLayoutDirty()){
			autoResize();
			__layoutDirty = false;
		}
		if(mouseEnabled){
			updateCollider();

			if(updateState){
				float d = mouse->getMouseWheelDelta();
				if(abs(d) > FLT_EPSILON){
					sweet::Event * e = new sweet::Event("mousewheel");
					e->setFloatData("delta", d);
					eventManager->triggerEvent(e);
				}

				if(!isHovered){
					in();
				}if(mouse->leftJustPressed()){
					down();
				}else if(mouse->leftJustReleased()){
					up();
				}
				updateState = false;
			}else{
				if(isHovered){
					out();
				}
				if(isDown && mouse->leftJustReleased()){
					isDown = false;
				}
			}
			NodeBulletBody::update(_step);
		}
		Entity::update(_step);
	}
}

bool NodeUI::__evaluateChildRenderFrames(){
	if(isLayoutDirty() || __renderFrameDirty){
		return true;
	}
	if(isVisible()){
		for(NodeChild * c : uiElements->children) {
			Transform * t = c->asTransform();
			if(t != nullptr){
				NodeUI * nodeUI = t->children.at(0)->asNodeUI();
				if(nodeUI != nullptr) {
					if(nodeUI->__evaluateChildRenderFrames()){
						invalidateRenderFrame();
						return true;
					}
				}
			}
		}
	}
	return false;
}

void NodeUI::__updateForTexture(Step * _step) {
	if(active){
		if(texturedPlane != nullptr){
			texturedPlane->update(_step);
		}

		__evaluateChildRenderFrames();

		if(isRenderFrameDirty()) {
			autoResize();
		}
	}
}

void NodeUI::setMarginLeft(float _margin){
	NodeUI * root = nodeUIParent;
	while(root->width.sizeMode == kAUTO && root->nodeUIParent != nullptr){
		root = root->nodeUIParent;
	}
	marginLeft.setSize(_margin, &root->width);
}

void NodeUI::setMarginRight(float _margin){
	NodeUI * root = nodeUIParent;
	while(root->width.sizeMode == kAUTO && root->nodeUIParent != nullptr){
		root = root->nodeUIParent;
	}
	marginRight.setSize(_margin, &root->width);
}

void NodeUI::setMarginTop(float _margin){
	NodeUI * root = nodeUIParent;
	while(root->width.sizeMode == kAUTO && root->nodeUIParent != nullptr){
		root = root->nodeUIParent;
	}
	marginTop.setSize(_margin, &root->height);
}

void NodeUI::setMarginBottom(float _margin){
	NodeUI * root = nodeUIParent;
	while(root->width.sizeMode == kAUTO && root->nodeUIParent != nullptr){
		root = root->nodeUIParent;
	}
	marginBottom.setSize(_margin, &root->height);
}

void NodeUI::setMargin(float _all){
	setMarginLeft(_all);
	setMarginRight(_all);
	setMarginBottom(_all);
	setMarginTop(_all);
}

void NodeUI::setMargin(float _leftAndRight, float _bottomAndTop){
	setMarginLeft(_leftAndRight);
	setMarginRight(_leftAndRight);
	setMarginBottom(_bottomAndTop);
	setMarginTop(_bottomAndTop);
}
void NodeUI::setMargin(float _left, float _right, float _bottom, float _top){
	setMarginLeft(_left);
	setMarginRight(_right);
	setMarginBottom(_bottom);
	setMarginTop(_top);
}

void NodeUI::setPaddingLeft(float _padding){
	NodeUI * root = nodeUIParent;
	while(root->width.sizeMode == kAUTO && root->nodeUIParent != nullptr){
		root = root->nodeUIParent;
	}
	paddingLeft.setSize(_padding, &root->width);
}

void NodeUI::setPaddingRight(float _padding){
	NodeUI * root = nodeUIParent;
	while(root->width.sizeMode == kAUTO && root->nodeUIParent != nullptr){
		root = root->nodeUIParent;
	}
	paddingRight.setSize(_padding, &root->width);
}

void NodeUI::setPaddingTop(float _padding){
	NodeUI * root = nodeUIParent;
	while(root->width.sizeMode == kAUTO && root->nodeUIParent != nullptr){
		root = root->nodeUIParent;
	}
	paddingTop.setSize(_padding, &root->height);
}

void NodeUI::setPaddingBottom(float _padding){
	NodeUI * root = nodeUIParent;
	while(root->width.sizeMode == kAUTO && root->nodeUIParent != nullptr){
		root = root->nodeUIParent;
	}
	paddingBottom.setSize(_padding, &root->height);
}

void NodeUI::setPadding(float _all){
	setPadding(_all, _all, _all, _all);
}

void NodeUI::setPadding(float _leftAndRight, float _bottomAndTop){
	setPadding(_leftAndRight, _leftAndRight, _bottomAndTop, _bottomAndTop);
}

void NodeUI::setPadding(float _left, float _right, float _bottom, float _top){
	setPaddingLeft(_left);
	setPaddingRight(_right);
	setPaddingBottom(_bottom);
	setPaddingTop(_top);
}

void NodeUI::setAlpha(float _alpha) {
	if(renderMode == kTEXTURE) {
		textureModeAlpha = _alpha;
	}else {
		ST_LOG_WARN("SetAlpha is not implemented for renderMode kENTITIES");
	}
}

float NodeUI::getTextureModeAlpha() const {
	return textureModeAlpha;
}

void NodeUI::setWidth(float _width){
	if(_width < 0){
		setAutoresizeWidth();
	}else if(_width <= 1.00001f){
		assert(nodeUIParent != nullptr);
		setRationalWidth(_width, nodeUIParent);
	}else{
		setPixelWidth(_width);
	}
}

void NodeUI::setHeight(float _height){
	if(_height < 0){
		setAutoresizeHeight();
	}else if(_height <= 1.00001f){
		assert(nodeUIParent != nullptr);
		setRationalHeight(_height, nodeUIParent);
	}else{
		setPixelHeight(_height);
	}
}

void NodeUI::setAutoresizeWidth(){
	width.setAutoSize();
	width.measuredSize = getContentsWidth();
	resizeChildrenWidth();
}
void NodeUI::setAutoresizeHeight(){
	height.setAutoSize();
	height.measuredSize = getContentsHeight();
	resizeChildrenHeight();
}

void NodeUI::setRationalWidth(float _rationalWidth, NodeUI * _root){
	width.setRationalSize(_rationalWidth, &_root->width);
	setMeasuredWidths();
}

void NodeUI::setRationalHeight(float _rationalHeight, NodeUI * _root){
	height.setRationalSize(_rationalHeight, &_root->height);
	setMeasuredHeights();
}

void NodeUI::setPixelWidth(float _pixelWidth){
	if(boxSizing == kBORDER_BOX){
		_pixelWidth -= getMarginLeft() + getPaddingLeft() + getPaddingRight() + getMarginRight();
	}
	width.setPixelSize(_pixelWidth);
	resizeChildrenWidth();
}

void NodeUI::setPixelHeight(float _pixelHeight){
	if(boxSizing == kBORDER_BOX){
		_pixelHeight -= getMarginBottom() + getPaddingBottom() + getPaddingTop() + getMarginTop();
	}
	height.setPixelSize(_pixelHeight);
	resizeChildrenHeight();
}

void NodeUI::setSquareWidth(float _rationalWidth){
	width.setRationalSize(_rationalWidth, &height);
	setMeasuredWidths();
}

void NodeUI::setSquareHeight(float _rationalHeight){
	height.setRationalSize(_rationalHeight, &width);
	setMeasuredHeights();
}

void NodeUI::resizeChildrenWidth(){
	for(auto c : uiElements->children) {
		Transform * trans = c->asTransform();
		if(trans != nullptr) {
			if(trans->children.size() > 0) {
				auto nui = trans->children.at(0);
				if(trans->children.at(0)->asNodeUI() != nullptr){
					nui->asNodeUI()->setMeasuredWidths();
				}
			}
		}
	}
}

void NodeUI::resizeChildrenHeight(){
	for(auto c : uiElements->children) {
		Transform * trans = c->asTransform();
		if(trans != nullptr) {
			if(trans->children.size() > 0) {
				auto nui = trans->children.at(0);
				if(trans->children.at(0)->asNodeUI() != nullptr){
					nui->asNodeUI()->setMeasuredHeights();
				}
			}
		}
	}
}

void NodeUI::setMeasuredWidths(){
	if(marginLeft.sizeMode == kRATIO){
		marginLeft.measuredSize = marginLeft.rationalTarget->getSize() * marginLeft.rationalSize;
	}if(paddingLeft.sizeMode == kRATIO){
		paddingLeft.measuredSize = paddingLeft.rationalTarget->getSize() * paddingLeft.rationalSize;
	}if(paddingRight.sizeMode == kRATIO){
		paddingRight.measuredSize = paddingRight.rationalTarget->getSize() * paddingRight.rationalSize;
	}if(marginRight.sizeMode == kRATIO){
		marginRight.measuredSize = marginRight.rationalTarget->getSize() * marginRight.rationalSize;
	}

	if(width.sizeMode == kRATIO){
		width.measuredSize = width.rationalTarget->getSize() * width.rationalSize;
		if(boxSizing == kBORDER_BOX){
			width.measuredSize -= marginLeft.getSize() + paddingLeft.getSize() + paddingRight.getSize() + marginRight.getSize();
		}
	}else if(width.sizeMode == kAUTO){
		width.measuredSize = getContentsWidth();
	}

	resizeChildrenWidth();
}

void NodeUI::setMeasuredHeights(){
	if(marginBottom.sizeMode == kRATIO){
		marginBottom.measuredSize = marginBottom.rationalTarget->getSize() * marginBottom.rationalSize;
	}if(paddingBottom.sizeMode == kRATIO){
		paddingBottom.measuredSize = paddingBottom.rationalTarget->getSize() * paddingBottom.rationalSize;
	}if(paddingTop.sizeMode == kRATIO){
		paddingTop.measuredSize = paddingTop.rationalTarget->getSize() * paddingTop.rationalSize;
	}if(marginTop.sizeMode == kRATIO){
		marginTop.measuredSize = marginTop.rationalTarget->getSize() * marginTop.rationalSize;
	}

	if(height.sizeMode == kRATIO){
		height.measuredSize = height.rationalTarget->getSize() * height.rationalSize;
		if(boxSizing == kBORDER_BOX){
			height.measuredSize -= marginBottom.getSize() + paddingBottom.getSize() + paddingTop.getSize() + marginTop.getSize();
		}
	}else if(height.sizeMode == kAUTO){
		height.measuredSize = getContentsHeight();
	}

	resizeChildrenHeight();
}

void NodeUI::setBackgroundColour(float _r, float _g, float _b, float _a){
	bgColour.r = _r - 1.f;
	bgColour.g = _g - 1.f;
	bgColour.b = _b - 1.f;
	bgColour.a = _a;
	invalidateRenderFrame();
}

float NodeUI::getMarginLeft(){
	return marginLeft.getSize();
}float NodeUI::getMarginRight(){
	return marginRight.getSize();
}float NodeUI::getMarginTop(){
	return marginTop.getSize();
}float NodeUI::getMarginBottom(){
	return marginBottom.getSize();
}

float NodeUI::getPaddingLeft(){
	return paddingLeft.getSize();
}float NodeUI::getPaddingRight(){
	return paddingRight.getSize();
}float NodeUI::getPaddingTop(){
	return paddingTop.getSize();
}float NodeUI::getPaddingBottom(){
	return paddingBottom.getSize();
}

float NodeUI::getWidth(){
	return width.getSize();
}float NodeUI::getHeight(){
	return height.getSize();
}

float NodeUI::getWidth(bool _includePadding, bool _includeMargin){
	float res = getWidth();
	if(_includePadding){
		res += getPaddingLeft() + getPaddingRight();
	}
	if(_includeMargin){
		res += getMarginLeft() + getMarginRight();
	}
	return res;
}

float NodeUI::getHeight(bool _includePadding, bool _includeMargin){
	float res = getHeight();
	if(_includePadding){
		res += getPaddingBottom() + getPaddingTop();
	}
	if(_includeMargin){
		res += getMarginBottom() + getMarginTop();
	}
	return res;
}

TriMesh * NodeUI::colliderMesh = nullptr;
void NodeUI::updateCollider(){
	glm::mat4 mat = background->meshTransform->getCumulativeModelMatrix();

	glm::vec4 verts[6] = {
		mat * glm::vec4(0,	1,0,1),
		mat * glm::vec4(1,	1,0,1),
		mat * glm::vec4(1,	0,0,1),
		mat * glm::vec4(1,	0,0,1),
		mat * glm::vec4(0,	0,0,1),
		mat * glm::vec4(0,	0,0,1)
	};

	if(colliderMesh == nullptr){
		colliderMesh = new TriMesh(true);
		for(unsigned long int i = 0; i < 6; ++i){
			colliderMesh->pushVert(Vertex(verts[i].x, verts[i].y, verts[i].z));
		}
		colliderMesh->incrementReferenceCount();
		colliderMesh->nodeName = "NodeUI collider mesh";
	}else{
		for(unsigned long int i = 0; i < 6; ++i){
			colliderMesh->vertices.at(i).x = verts[i].x;
			colliderMesh->vertices.at(i).y = verts[i].y;
			colliderMesh->vertices.at(i).z = verts[i].z;
		}
	}

	if(shape != nullptr){
		delete shape;
		shape = nullptr;
	}
	setColliderAsMesh(colliderMesh, true);

	if(body != nullptr){
		world->world->removeRigidBody(body);
		body->setCollisionShape(shape);
		world->world->addRigidBody(body);
	}else{
		createRigidBody(0);
	}
}

void NodeUI::autoResize(){
	/*if(width.sizeMode == kAUTO){
	width.measuredSize = getContentsWidth();
	}
	if(height.sizeMode == kAUTO){
	height.measuredSize = getContentsHeight();
	}*/
	setMeasuredWidths();
	setMeasuredHeights();

	// Adjust the size of the background
	background->firstParent()->scale(getWidth(true, false), getHeight(true, false), 1.0f, false);
	repositionChildren();
}
float NodeUI::getContentsWidth(){
	float w = 0.0f;
	// take the maximum of the width of the contents
	for(unsigned long int i = 0; i < uiElements->children.size(); ++i) {
		Transform * trans = uiElements->children.at(i)->asTransform();
		if(trans != nullptr) {
			if(trans->children.size() > 0) {
				w = std::max(w, trans->children.at(0)->asNodeUI()->getWidth(true, true));
			}
		}
	}
	return w;
}
float NodeUI::getContentsHeight(){
	float h = 0.0f;
	// take the maximum of the height of the contents66
	for(unsigned long int i = 0; i < uiElements->children.size(); ++i) {
		Transform * trans = uiElements->children.at(i)->asTransform();
		if(trans != nullptr) {
			if(trans->children.size() > 0) {
				h = std::max(h, trans->children.at(0)->asNodeUI()->getHeight(true, true));
			}
		}
	}
	return h;
}

void NodeUI::repositionChildren(){
	margin->translate(getMarginLeft(), getMarginBottom(), 0, false);
	padding->translate(getPaddingLeft(), getPaddingBottom(), 0, false);
}

void NodeUI::setUpdateState(bool _newState){
	updateState = _newState;
}

void NodeUI::setRenderMode(RenderMode _newRenderMode){
	renderMode = _newRenderMode;
	invalidateRenderFrame();
}

bool NodeUI::isRenderFrameDirty(){
	return renderMode == kTEXTURE && __renderFrameDirty;
}

void NodeUI::invalidateRenderFrame(){
	__renderFrameDirty = true;
}

bool NodeUI::isLayoutDirty(){
	return __layoutDirty;
}

void NodeUI::invalidateLayout() {
	doRecursivelyOnUIChildren([](NodeUI * _this){
		_this->__layoutDirty = true;
		_this->invalidateRenderFrame();
	}, true);
}
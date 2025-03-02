#pragma once

#include <node\Node.h>

enum SizeMode{
	kPIXEL, // exact pixel size
	kRATIO, // percentage of other UIUnit's size
	kAUTO // based on children
};

class UIUnit : public Node{
public:
	const UIUnit * rationalTarget;
	// if sizeMode == kPIXEL, this is the size
	float pixelSize;
	// if sizeMode == kRATIO, this is the ratio of the containing element's size to occupy
	float rationalSize;
	// if sizeMode == kRATIO or kAUTO, this is the actual measured size based on the parent/contents
	float measuredSize;

	SizeMode sizeMode;

	UIUnit();
	
	// sets the width
	// value < 0: the dimension will be auto-resized
	// 0 <= value <= 1: the dimension will be sized as a ratio of _target's size
	// value > 1: the dimension will be sized in units (typically screen pixels)
	void setSize(float _size, UIUnit * _target);

	// sets sizeMode to kPIXEL
	// the dimension will be sized as a ratio of its parent's size
	void setPixelSize(float _size);

	// sets sizeMode to kRATIO
	// the dimension will be sized as a ratio of its parent's size
	// if _parent is nullptr, only sets the member variable and doesn't change the current size
	void setRationalSize(float _size, const UIUnit * const _target);
	
	// sets sizeMode to kAUTO
	// the dimension will be sized as a ratio of its parent's size
	void setAutoSize();

	// if kPIXEL, returns the pixelSize
	// otherwise, returns the measuredSize
	 float getSize() const;
};
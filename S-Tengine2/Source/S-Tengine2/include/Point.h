#pragma once

#include <node\Node.h>

namespace sweet {
	class Point : public Node{
	public:	
		float x, y;

		Point(float _x, float _y);
		~Point();
	};
};
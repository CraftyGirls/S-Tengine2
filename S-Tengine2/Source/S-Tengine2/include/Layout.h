#pragma once

#include <NodeUI.h>

class Layout abstract : public NodeUI {
public:

	Layout(BulletWorld * _world);
	virtual ~Layout();

	virtual void layoutChildren() = 0;

	virtual void update(Step * _step) override;
};
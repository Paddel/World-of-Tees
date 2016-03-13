#pragma once

#include <base/vmath.h>

class CColLaser
{
private:

public:
	virtual vec2 From() = 0;
	virtual vec2 To() = 0;
	virtual bool Active() { return true; }
};
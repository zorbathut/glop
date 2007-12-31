#include "../include/Collisions.h"
#include "../include/MathUtil.h"

Box::Box(float x, float y, float width, float height) : _x(x), _y(y), _width(width), _height(height) {
	_x2 = _x + _width;
	_y2 = _y + _height;
}

Box::~Box() {
}

bool Box::Collides(const Box& box) const {
	if (box.x() >= _x2 || box.x2() <= _x || box.y() >= _y2 || box.y2() <= _y)
		return false;
	return true;
}

bool Box::Collides(const Circle& circle) const {
	Box box(circle.x() - circle.radius(), circle.y() - circle.radius(), circle.radius() * 2, circle.radius() * 2);
	
	// if a simple box/box collision fails, then the circle couldnt be colliding
	if (!Collides(box))
		return false;

	// find out if the circle is within the regions specified with X's
	//			x|    |x
	//		   ----------
	//			 |    |
	//		   ----------
	//          x|    |x
	// if it is, find the distance from the center of the circle to the corner of the box.
	// if the radius is smaller than that value, the circle is colliding

	if (circle.x() < _x && circle.y() < _y) {
		if (DistSquared(circle.x(), circle.y(), _x, _y) < Sqr(circle.radius()))
			return true;
		return false;
	}
	else if (circle.x() > _x2 && circle.y() < _y) {
		if (DistSquared(circle.x(), circle.y(), _x2, _y) < Sqr(circle.radius()))
			return true;
		return false;
	}
	else if (circle.x() < _x && circle.y() > _y2) {
		if (DistSquared(circle.x(), circle.y(), _x, _y2) < Sqr(circle.radius()))
			return true;
		return false;
	}
	else if (circle.x() > _x2 && circle.y() > _y2) {
		if (DistSquared(circle.x(), circle.y(), _x2, _y2) < Sqr(circle.radius()))
			return true;
		return false;
	}

	return true;
}


Circle::Circle(float x, float y, float radius) : _x(x), _y(y), _radius(radius) {
}

Circle::~Circle() {
}

bool Circle::Collides(const Circle& circle) const {
	return DistSquared(circle.x(), circle.y(), _x, _y) < Sqr(circle.radius() + _radius);
}

bool Circle::Collides(const Box& box) const {
	return box.Collides(*this);
}
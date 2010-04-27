#ifndef COLLLISIONS_H__
#define COLLLISIONS_H__

class Box;
class Circle;

class Box
{
	float _x;
	float _y;
	float _x2;
	float _y2;
	float _width;
	float _height;

	Box();

public:

	Box(float x, float y, float width, float height);
	~Box();

	float x() const {return _x;}
	float y() const {return _y;}
	float x2() const {return _x2;}
	float y2() const {return _y2;}
	float width() const {return _width;}
	float height() const {return _height;}

	bool Collides(const Box& box) const;
	bool Collides(const Circle& circle) const;
};


class Circle
{
	float _x;
	float _y;
	float _radius;

	Circle();

public:

	Circle(float x, float y, float raduis);
	~Circle();

	float x() const {return _x;}
	float y() const {return _y;}
	float radius() const {return _radius;}

	bool Collides(const Circle& circle) const;
	bool Collides(const Box& Box) const;
};



#endif // COLLLISIONS_H__

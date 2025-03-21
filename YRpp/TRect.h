#pragma once


template<typename T>
class TRect
{
public:
	COMPILETIMEEVAL TRect() : X(0), Y(0), Width(0), Height(0) { }
	COMPILETIMEEVAL TRect(T x, T y, T w, T h) : X(x), Y(y), Width(w), Height(h) {}
	COMPILETIMEEVAL TRect(const TRect &that) : X(that.X), Y(that.Y), Width(that.Width), Height(that.Height) {}

	COMPILETIMEEVAL TRect &operator=(const TRect &that)
	{
		if (this != &that) {
			X = that.X;
			Y = that.Y;
			Width = that.Width;
			Height = that.Height;
		}
		return *this;
	}

	COMPILETIMEEVAL bool operator==(const TRect &that) const
	{
		return (that.X == X) && (that.Width == Width)
			&& (that.Y == Y) && (that.Height == Height);
	}

	COMPILETIMEEVAL bool operator!=(const TRect &that) const
	{
		return (that.X != X) && (that.Width != Width)
			&& (that.Y != Y) && (that.Height != Height);
	}

public:
	T X;
	T Y;
	T Width;
	T Height;
};

template<typename T>
bool operator==(const TRect<T> &left, const TRect<T> &right)
{
	return (left.X == right.X)
		&& (left.Width == right.Width)
		&& (left.Y == right.Y)
		&& (left.Height == right.Height);
}

template<typename T>
bool operator!=(const TRect<T> &left, const TRect<T> &right)
{
	return (left.X != right.X)
		&& (left.Width != right.Width)
		&& (left.Y != right.Y)
		&& (left.Height != right.Height);
}

template<typename T>
TRect<T> operator+(const TRect<T> &left, const TRect<T> &right)
{
	TRect<T> result = left;
	TRect<T> tmp = right;
	result += tmp;
	return result;
}

template<typename T>
TRect<T> operator-(const TRect<T> &left, const TRect<T> &right)
{
	TRect<T> result = left;
	TRect<T> tmp = right;
	result -= tmp;
	return result;
}

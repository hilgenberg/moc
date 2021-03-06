#pragma once

struct Rect
{
	Rect() : x(0), y(0), w(0), h(0) {}
	Rect(int x, int y, int w, int h) : x(x), y(y), w(w), h(h) {}
	
	Rect inset(int d) const { return Rect(x+d, y+d, w-2*d, h-2*d); }
	void center(int W, int H) { x = std::max(0, (W-w)/2); y = std::max(0, (H-h)/2); }

	void set(int x_, int y_, int w_, int h_) { x=x_; y=y_; w=w_; h=h_; }
	
	int x, y;
	int w, h;
	int x1() const { return x+w; }
	int y1() const { return y+h; }

	bool contains(int ex, int ey) const
	{
		return ex >= x && ey >= y && ex < x+w && ey < y+h;
	}
};

inline Rect operator+ (const Rect &a, const Rect &b)
{
	Rect r; // no support for negative w or h or empty Rects!
	r.x = std::min(a.x, b.x);
	r.y = std::min(a.y, b.y);
	r.w = std::max(a.x1(), b.x1()) - r.x;
	r.h = std::max(a.y1(), b.y1()) - r.y;
	return r;
}

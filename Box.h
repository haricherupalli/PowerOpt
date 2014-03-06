#ifndef __BOX_H__
#define __BOX_H__

namespace POWEROPT {

  class Box {
  public:
    //constructors
    Box() {}
	  Box(int l, int b, int r, int t):lx(l), by(b), rx(r), ty(t) {}
    //modifiers
    void set(int l, int b, int r, int t)
    {
      lx = l;
      by = b;
      rx = r;
      ty = t;
    }
    //accossers
    int left() { return lx; }
    int bottom() { return by; }
    int right() { return rx; }
    int top() { return ty; }
  private:
    int lx;
    int by;
    int rx;
    int ty;
  };
}

#endif //__BOX_H__

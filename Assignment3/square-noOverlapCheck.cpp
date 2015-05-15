/*
 *  Main author:
 *     Christian Schulte <cschulte@kth.se>
 *
 *  Copyright:
 *     Christian Schulte, 2009
 *
 *  Permission is hereby granted, free of charge, to any person obtaining
 *  a copy of this software and associated documentation files (the
 *  "Software"), to deal in the Software without restriction, including
 *  without limitation the rights to use, copy, modify, merge, publish,
 *  distribute, sublicense, and/or sell copies of the Software, and to
 *  permit persons to whom the Software is furnished to do so, subject to
 *  the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 *  LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 *  OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 *  WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include <gecode/int.hh>

using namespace Gecode;
using namespace Gecode::Int;

// The no-overlap propagator
class NoOverlap : public Propagator {
protected:
    // The x-coordinates
    ViewArray<IntView> x;
    // The width (array)
    int* w;
    // The y-coordinates
    ViewArray<IntView> y;
    // The heights (array)
    int* h;
public:
    // Create propagator and initialize
    NoOverlap(Home home,
              ViewArray<IntView>& x0, int w0[],
              ViewArray<IntView>& y0, int h0[])
    : Propagator(home), x(x0), w(w0), y(y0), h(h0) {
        x.subscribe(home,*this,PC_INT_BND);
        y.subscribe(home,*this,PC_INT_BND);
    }
    // Post no-overlap propagator
    static ExecStatus post(Home home,
                           ViewArray<IntView>& x, int w[],
                           ViewArray<IntView>& y, int h[]) {
        // Only if there is something to propagate
        if (x.size() > 1)
            (void) new (home) NoOverlap(home,x,w,y,h);
        return ES_OK;
    }
    
    // Copy constructor during cloning
    NoOverlap(Space& home, bool share, NoOverlap& p)
    : Propagator(home,share,p) {
        x.update(home,share,p.x);
        y.update(home,share,p.y);
        // Also copy width and height arrays
        w = home.alloc<int>(x.size());
        h = home.alloc<int>(y.size());
        for (int i=x.size(); i--; ) {
            w[i]=p.w[i]; h[i]=p.h[i];
        }
    }
    // Create copy during cloning
    virtual Propagator* copy(Space& home, bool share) {
        return new (home) NoOverlap(home,share,*this);
    }
    
    // Return cost (defined as cheap quadratic)
    virtual PropCost cost(const Space&, const ModEventDelta&) const {
        return PropCost::quadratic(PropCost::LO,2*x.size());
    }
    
    // Perform propagation
    virtual ExecStatus propagate(Space& home, const ModEventDelta&) {
        
        int countAssigned = 0;
        
        for (int i = 0; i < x.size(); i++) {
            if (x[i].assigned() && y[i].assigned()) {
                // Count the assigned x,y pairs
                countAssigned++;
                for (int j = i+1; j < x.size(); j++) {
                    
                    // If j overlaps with i return failure
                    // otherwise the values that conflict are removed.
                    if (    me_failed(x[j].lq(home, x[i].val()-w[j]))
                        &&  me_failed(x[j].gq(home, x[i].val()+w[i]))
                        &&  me_failed(y[j].lq(home, y[i].val()-h[j]))
                        &&  me_failed(y[j].gq(home, y[i].val()+h[i]))) {
                        return ES_FAILED;
                    }
                }
            }
        }
        // When all the variables are assigned, return subsumption
        // Otherwise the propagator is at fixpoint
        if(countAssigned == x.size())
            return home.ES_SUBSUMED(*this);
        
        return ES_FIX;    }
    
    // Dispose propagator and return its size
    virtual size_t dispose(Space& home) {
        x.cancel(home,*this,PC_INT_BND);
        y.cancel(home,*this,PC_INT_BND);
        (void) Propagator::dispose(home);
        return sizeof(*this);
    }
};

/*
 * Post the constraint that the rectangles defined by the coordinates
 * x and y and width w and height h do not overlap.
 *
 * This is the function that you will call from your model. The best
 * is to paste the entire file into your model.
 */



#include <gecode/driver.hh>
#include <gecode/int.hh>
#include <gecode/minimodel.hh>

using namespace Gecode;

int n;

class Square : public Script {
public:
    
    IntVar s;       // size of square (w=h)
    IntVarArray x;  // x axis
    IntVarArray y;  // y axis
    
    Square(const SizeOptions& opt)
    : Script(opt), x(*this, n, 0, sum(n)), y(*this, n, 0 , sum(n)) {
        
        //initialize s
        s = IntVar(*this, floor(sqrt(n*(n+1)*(2*n+1)/6)), sum(n));
        
        
        // squares must be inside the enclosing square
        for (int i = 0; i < n; i++) {
            rel(*this, (x[i] + size(i)) <= s);
            rel(*this, (y[i] + size(i)) <= s);
        }
        

        // the sum of the sizes of the squares occupying space at
        // any column and row must be less than or equal to s.
        IntArgs sizes = IntArgs(n);
        for(int i =0; i<n;i++){
            sizes[i]= size(i);
        }
        // As s might not be assigned a value yet, you might have consider all columns (and rows) from 0 to s.max().
        for(int count=0; count<s.max(); count++){
            BoolVarArgs bx(*this,n,0,1);
            BoolVarArgs by(*this,n,0,1);
            for(int i=0; i<n; i++){
                dom(*this, x[i], count, count+size(i),bx[i]);
                dom(*this, y[i], count, count+size(i),by[i]);
            }
            linear(*this, sizes, by, IRT_LQ, s);
            linear(*this, sizes, bx, IRT_LQ, s);
        }
        
        // s1 is left of s2 or
        // s2 is left of s1 or
        // s1 is above s2 or
        // s2 is above s1
        // with noOverlap constraint
        nooverlap(*this, x, sizes, y, sizes );
        
        
        // Symmetry removal
        rel(*this, x[0] <= 1+((s-size(0))/2));
        rel(*this, y[0] <= 1+((s-size(0))/2));
        
        // Empty strip dominance
        for (int i = 0; i < n; ++i) {
            int gap = 0;
            if (size(i) == 2 || size(i) == 4) {
                gap = 2;
            }
            else if (size(i) == 3 || (size(i) >= 5 && size(i) <= 8)) {
                gap = 3;
            }
            else if (size(i) <= 11) {
                gap = 4;
            }
            else if (size(i) <= 17) {
                gap = 5;
            }
            else if (size(i) <= 21) {
                gap = 6;
            }
            else if (size(i) <= 29) {
                gap = 7;
            }
            else if (size(i) <= 34) {
                gap = 8;
            }
            else if (size(i) <= 44) {
                gap = 9;
            }
            else if (size(i) <= 45) {
                gap = 10;
            }
            
            if (gap != 0) {
                rel(*this, x[i] != gap);
                rel(*this, y[i] != gap);
            }
        }
        
        // Branching on min s wil give us the optimal solution first
        // Biggest squares are placed first from the way our model is constructed (i=0 is the largest square)
        // Branching x and y from left to right and bottom to top
        branch(*this, s, INT_VAL_MIN());
        branch(*this, x, INT_VAR_NONE(), INT_VAL_MIN());
        branch(*this, y, INT_VAR_NONE(), INT_VAL_MIN());
    }
    
    /// Constructor for cloning
    Square(bool share, Square& sq) : Script(share,sq) {
        x.update(*this, share, sq.x);
        y.update(*this, share, sq.y);
        s.update(*this, share, sq.s);
    }
    
    /// Perform copying during cloning
    virtual Space*
    copy(bool share) {
        return new Square(share,*this);
    }
    
    //returns the size of square i
    static int size(int i){
        return n-i;
    }
    
    // returns the sum of all square sizes
    static int sum(int n){
        int sum = 0;
        for(int i=0; i<n; i++){
            sum += n-i;
        }
        return sum;
    }
    
    /// Print solution
    virtual void print(std::ostream& os) const {
        os << "\t";
        os << "Smallest S = " << s << std::endl << "\t";
        os << "For N = " << n << std::endl << "\t";
        for (int i = 0; i < n; i++) {
                os << "square with size " << size(i) << "\tPosition: " <<  x[i] << "," << y[i];
                os << std::endl << "\t";
            
        }
        os << std::endl;
    }
    void nooverlap(Home home,
                   const IntVarArgs& x, const IntArgs& w,
                   const IntVarArgs& y, const IntArgs& h) {
        // Check whether the arguments make sense
        if ((x.size() != y.size()) || (x.size() != w.size()) ||
            (y.size() != h.size()))
            throw ArgumentSizeMismatch("nooverlap");
        // Never post a propagator in a failed space
        if (home.failed()) return;
        // Set up array of views for the coordinates
        ViewArray<IntView> vx(home,x);
        ViewArray<IntView> vy(home,y);
        // Set up arrays (allocated in home) for width and height and initialize
        int* wc = static_cast<Space&>(home).alloc<int>(x.size());
        int* hc = static_cast<Space&>(home).alloc<int>(y.size());
        for (int i=x.size(); i--; ) {
            wc[i]=w[i]; hc[i]=h[i];
        }
        // If posting failed, fail space
        if (NoOverlap::post(home,vx,wc,vy,hc) != ES_OK)
            home.fail();
    }
};

int main(int argc, char* argv[]) {
    SizeOptions opt("Square");
    int in;
    std::cout << "Please enter the number of squares to pack" << std::endl;
    std::cin >> in;
    while(std::cin.fail() || in < 0) {
        std::cout << "Invalid input, please enter a positive integer" << std::endl;
        std::cin.clear();
        std::cin.ignore(256,'\n');
        std::cin >> in;
    }
    opt.size(in);
    n = opt.size();
    opt.parse(argc,argv);
    Script::run<Square,DFS,SizeOptions>(opt);
    return 0;
}

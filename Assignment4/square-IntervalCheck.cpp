
#include <gecode/driver.hh>
#include <gecode/int.hh>
#include <gecode/minimodel.hh>
#include "interval.cpp"
using namespace Gecode;

int n;

class Square : public Script {
public:

    IntVar s;       // size of square (w=h)
    IntVarArray x;  // x axis
    IntVarArray y;  // y axis
    
    Square(const SizeOptions& opt)
    : Script(opt), x(*this, n, 0, sum(n)), y(*this, n, 0 , sum(n)) {
        
        // Smallest square is ignored (by using i < n-1 in the for loops)
        
        //initialize s
        s = IntVar(*this, floor(sqrt(n*(n+1)*(2*n+1)/6)), sum(n));


        // squares must be inside the enclosing square
        for (int i = 0; i < n-1; i++) {
            rel(*this, (x[i] + size(i)) <= s);
            rel(*this, (y[i] + size(i)) <= s);
        }
        
        // s1 is left of s2 or
        // s2 is left of s1 or
        // s1 is above s2 or
        // s2 is above s1
        for (int i=0; i<n-1; i++) {
            for(int j=i+1; j<n-1; j++){

                IntVar left(*this,0, sum(n)+size(i));
                IntVar right(*this,0, sum(n)+size(j));
                IntVar above(*this,0, sum(n)+size(i));
                IntVar below(*this,0, sum(n)+size(j));
                
                BoolVarArgs b(*this,4,0,1);
                

                rel(*this, left == x[i]+size(i));
                rel(*this, right == x[j]+size(j));
                rel(*this, above == y[i]+size(i));
                rel(*this, below == y[j]+size(j));

                rel(*this, left, IRT_LQ, x[j], b[0]);
                rel(*this, right, IRT_LQ, x[i], b[1]);
                rel(*this, above, IRT_LQ, y[j], b[2]);
                rel(*this, below, IRT_LQ, y[i], b[3]);
                
                linear(*this, b, IRT_GQ, 1);

            }
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
            for(int i=0; i<n-1; i++){
                dom(*this, x[i], count, count+size(i),bx[i]);
                dom(*this, y[i], count, count+size(i),by[i]);
            }
            linear(*this, sizes, by, IRT_LQ, s);
            linear(*this, sizes, bx, IRT_LQ, s);
        }

        // Symmetry removal
//        rel(*this, x[0] <= 1+((s-size(0))/2));
//        rel(*this, y[0] <= 1+((s-size(0))/2));
        
        // Empty strip dominance
        for (int i = 0; i < n-1; ++i) {
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
        double p = 0.5;
        interval(*this, x, sizes, p);
        interval(*this, y, sizes, p);
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
            if (i!=n-1){
                os << "square with size " << size(i) << "\tPosition: " <<  x[i] << "," << y[i];
                os << std::endl << "\t";
            } else {
                os << "Smallest piece is guaranteed to have a free position in the enclosing square" ;
            }
        }
        os << std::endl;
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
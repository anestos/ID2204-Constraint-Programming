#include <gecode/driver.hh>
#include <gecode/int.hh>
#include <gecode/minimodel.hh>

using namespace Gecode;

class Life : public Script {
public:
    /// Size
    BoolVarArray q;
    
    /// The actual problem
    Life(const SizeOptions& opt)
    : Script(opt), q(*this,(opt.size()+4)*(opt.size()+4),0,1) {
        
        // the plus 4 is to add the border with 2 cells thickness
        int n = opt.size()+4;
        Matrix<BoolVarArgs> board(q, n, n);

        // set the value on the border to 0
        for (int i = 0; i<2; i++){
            linear(*this, board.row(i), IRT_EQ, 0);
            linear(*this, board.row(n-1-i), IRT_EQ, 0);
            linear(*this, board.col(i), IRT_EQ, 0);
            linear(*this, board.col(n-1-i), IRT_EQ, 0);
        }
        
        // we start on the inner row of the border.
        // the outer will never change if the inner doesn't change
        for (int i =1; i<n-1; i++){
            for (int j = 1; j < n-1; j++) {
                BoolVarArray neighbours(*this, 8);
                int counter = 0;
                // the 8 neighbours of each cell
                for (int m = i-1; m <= i+1; m++){
                    for (int l = j-1; l <= j+1; l++){
                        if (!(m==i &&  l==j)){
                            neighbours[counter] = board(m,l);
                            counter++;
                        }
                    }
                }
                // the sum of neighbours of a cell
                IntVar sumOfAliveNeighbours(*this, 0, 8);
                rel(*this, sum(neighbours) == sumOfAliveNeighbours);
                // if the cell is alive it must have 2 or 3 alive neighbours
                rel(*this, board(i,j) >> (sumOfAliveNeighbours==2 || sumOfAliveNeighbours==3));
                // if the cell is dead it cannot have 3 neighbours
                rel(*this, !board(i,j) >> (sumOfAliveNeighbours !=3));
           
            }
            
        }

        //branching
         branch(*this, q, INT_VAR_SIZE_MAX(), INT_VAL_MAX());
        
    }
    // constraint for finding better solutions
    virtual void constrain(const Space& _b) {
        const Life& b = static_cast<const Life&>(_b);
        rel(*this, sum(q) > sum(b.q));
    }
    
    /// Constructor for cloning
    Life(bool share, Life& s) : Script(share,s) {
        q.update(*this, share, s.q);
    }
    
    /// Perform copying during cloning
    virtual Space*
    copy(bool share) {
        return new Life(share,*this);
    }
    
    /// Print solution
    virtual void
    print(std::ostream& os) const {
        int size = sqrt(q.size()+4);
        int alive = 0;
        
        std::cout << "\t";
        for (int i = 0; i < q.size(); i++) {
            if (q[i].val()==1){
                alive++;
            }
            std::cout << q[i] << "\t";
            if ((i+1) % size == 0)
                std::cout << std::endl << "\t";
        }
        std::cout << "Number of Alive:" << alive << std::endl;
        std::cout << std::endl;
    }
};

int main(int argc, char* argv[]) {
    SizeOptions opt("Life");
    int in;
    std::cout << "Please enter the number of size of board" << std::endl;
    std::cin >> in;
    while(std::cin.fail() || in < 0) {
        std::cout << "Invalid input, please enter a positive integer" << std::endl;
        std::cin.clear();
        std::cin.ignore(256,'\n');
        std::cin >> in;
    }
    opt.size(in);
    opt.solutions(0);
    opt.parse(argc,argv);
    Script::run<Life,BAB,SizeOptions>(opt);
    return 0;
    
    
//    SizeOptions opt("Life");
//    opt.size(8);
//    Life* m = new Life(opt);
//    BAB<Life> e(m);
//    delete m;
//    while (Life* s = e.next()) {
//        s->print(); delete s;
//    }
//    return 0;
}
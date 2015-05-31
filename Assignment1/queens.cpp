#include <gecode/driver.hh>
#include <gecode/int.hh>
#include <gecode/minimodel.hh>

using namespace Gecode;

class Queens : public Script {
public:
    /// Position of queens on boards
    IntVarArray q;

    /// The actual problem
    Queens(const SizeOptions& opt)
    : Script(opt), q(*this,opt.size()*opt.size(),0,1) {
        
        Matrix<IntVarArgs> board(q,opt.size(),opt.size());
        for(int i=0; i<opt.size(); i++){

            linear(*this, board.row(i),IRT_EQ,1);
            linear(*this, board.col(i),IRT_EQ,1);
        }
        
        // Diagonal
        for (int j = 0; j < opt.size(); ++j) {
            IntVarArray diagonial(*this, opt.size()-j);
            IntVarArray diagionialMirror(*this, opt.size()-j);
            IntVarArray diagonialInvert(*this, opt.size()-j);
            IntVarArray diagionialInvertMirror(*this, opt.size()-j);

            for (int i = 0; i+j < opt.size(); ++i) {
                diagonial[i] = board(i+j, i);
                diagionialMirror[i] = board(i, i+j);
                diagonialInvert[i] = board((opt.size() - 1) - (i + j), i);
                diagionialInvertMirror[i] = board((opt.size() - 1) - i , i + j);
  
                
                
            }
            linear(*this, diagonial, IRT_LQ, 1);
            linear(*this, diagionialMirror, IRT_LQ, 1);
            linear(*this, diagonialInvert, IRT_LQ, 1);
            linear(*this, diagionialInvertMirror, IRT_LQ, 1);


        }


       // branch(*this, q, INT_VAR_SIZE_MAX(), INT_VAL_MAX());
        branch(*this, q, INT_VAR_DEGREE_MAX(), INT_VAL_MAX());

    }
    
    /// Constructor for cloning 
    Queens(bool share, Queens& s) : Script(share,s) {
        q.update(*this, share, s.q);
    }
    
    /// Perform copying during cloning
    virtual Space*
    copy(bool share) {
        return new Queens(share,*this);
    }
    
    /// Print solution
    virtual void
    print(std::ostream& os) const {
        int size = sqrt(q.size());

        os << "\t";
        for (int i = 0; i < q.size(); i++) {
            os << q[i] << ", ";
            if ((i+1) % size == 0)
                os << std::endl << "\t";
        }
        os << std::endl;
    }
};

int main(int argc, char* argv[]) {
    SizeOptions opt("Queens");
    opt.iterations(500);
    opt.size(8);
 
    
    //#if defined(GECODE_HAS_QT) && defined(GECODE_HAS_GIST)
    //    QueensInspector ki;
    //    opt.inspect.click(&ki);
    //#endif
    
    opt.parse(argc,argv);
    Script::run<Queens,DFS,SizeOptions>(opt);
    return 0;
}

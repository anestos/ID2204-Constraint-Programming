
#include <iostream>
#include <string>
#include <gecode/int.hh>
#include <gecode/search.hh>
#include <gecode/gist.hh>
#include <gecode/driver.hh>
#include <gecode/minimodel.hh>
#include "A1.cpp"


using namespace Gecode;
extern int examples[][9][9];

class SudokuSolver : public Script{
    protected :
        IntVarArray n;
    public:
    SudokuSolver(const Options& opt) : Script(opt), n(*this, 81, 1, 9){

        //Prompt user to select a puzzle
        int in;
        std::cout << "Please enter puzzle number (0-17): ";
        std::cin >> in;


        
        //Get example from A1.cpp
        static int example [9][9];
        for (int i=0; i<9; i++){
            for (int j = 0; j<9 ; j++){
                example[i][j] = examples[in][i][j];
            }
        }
        
        //Constraints

        //Example to solve
        static int findSolution [81];
        for (int i=0; i<9; i++){
            for (int j = 0; j<9 ; j++){
                findSolution[(i*9)+j] = example[i][j];
                if( example[i][j] != 0){
                    rel(*this, n[(i*9)+j], IRT_EQ, example[i][j]);
                }
            }
        }

        Matrix<IntVarArgs> mat(n,9,9);

        // Rows and columns constraints
        for (int i=0; i<9; i++){
            distinct(*this, mat.row(i), opt.icl());
            distinct(*this, mat.col(i), opt.icl());
        }


        // 3x3 blocks constraint
        for (int i=0; i<9; i+=3){
            for (int j=0; j<9; j+=3){
                distinct(*this, mat.slice(i, i+3, j, j+3), opt.icl());
            }
        }

        //Branching
        branch(*this, n, INT_VAR_SIZE_MIN(), INT_VAL_MIN());
        
        
    }
    
    
    // Constructor for cloning
    SudokuSolver(bool share, SudokuSolver& s) : Script(share, s) {
        n.update(*this, share, s.n);
    }
    // Perform copying during cloning
    virtual Space* copy(bool share) {
        return new SudokuSolver(share,*this);
    }
    
    /// Print solution
    virtual void
    print(std::ostream& os) const {
        os << '\t';
        for (int i = 0; i<81; i++) {
            for (int j=0; j<9; j++) {
                    if (j+1<10)
                        os << n[i] << " ";
                    else
                        os << (char)(j+1+'A'-10) << " ";
                    break;
            }
            if((i+1)%(9) == 0)
                os << std::endl << '\t';
        }
    }
    
    
};

int main( int argc, char* argv[]){
    Options opt("Sudoku");
    opt.solutions(0);
    opt.iterations(200000);
    
    opt.parse(argc,argv);
    Script::run<SudokuSolver,DFS,Options>(opt);

    return 0;
}
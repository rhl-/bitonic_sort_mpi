
#include <boost/mpi.hpp>
//#include <diy/serialization.hpp>
#include "bitonic_sort.hpp"
#include <vector>
#include <random>
#include <sstream>

typedef std::vector< int> Vector;
namespace mpi = boost::mpi;

int main( int argc, char** argv){
mpi::environment environment( argc, argv);
mpi::communicator world;

std::random_device rd; 
std::mt19937 gen(rd());
std::vector< int> data( 10);
std::generate(data.begin(), data.end(), gen);
for( auto& i : data){ i = i %  1024; }
std::cout << data.size() << std::endl;
std::stringstream ss;
std::vector< std::vector< int> > orig_data;
data.push_back( world.rank()) ;
mpi::gather( world, data, orig_data, 0);
std::vector< int> correct_answer;
if( world.rank() == 0){
for( auto& p : orig_data){
    int rank = p.back();
    p.pop_back();
    ss << rank << " "; 
    correct_answer.insert( correct_answer.end(), p.begin(), p.end());
    for( auto & i : p){ ss <<  i << " "; } 
    ss << std::endl;
}
std::sort( correct_answer.begin(), correct_answer.end());
std::cout << ss.str() << std::endl;
}
data.pop_back();

ss << "Rank " << world.rank() << ": " << " calling bitonic sort";
distributed::bitonic_sort( world, data.begin(), data.end());
ss.str("");
ss << "Rank " << world.rank() << ": ";
for( auto& i : data){ ss << i << " "; }
ss << std::endl;
std::cout << ss.str() << std::endl;
if( world.rank() == 0){
for( auto & i : correct_answer){
	std::cout << i << " ";
}
std::cout << std::endl;
}
return 0;
}

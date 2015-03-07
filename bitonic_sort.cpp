
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
if( world.rank() == 0){
for( auto& p : orig_data){ 
    int rank = p.back();
    p.pop_back();
    ss << rank << " ";
    for( auto & i : p){ ss <<  i << " "; } 
    ss << std::endl;
}
std::cout << ss.str() << std::endl;
}
distributed::bitonic_sort( world, data.begin(), data.end());
ss.str("");
ss << "Rank " << world.rank() << ": ";
for( auto& i : data){ ss << i << " "; }
ss << std::endl;
std::cout << ss.str() << std::endl;

return 0;
}

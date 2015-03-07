
#include <diy/mpi.hpp>
#include <diy/serialization.hpp>
#include "bitonic_sort.hpp"
#include <vector>
#include <random>
#include <sstream>

typedef std::vector< int> Vector;
namespace mpi = diy::mpi;

int main( int argc, char** argv){
mpi::environment environment( argc, argv);
mpi::communicator world;

std::random_device rd; 
std::mt19937 gen(rd());
std::vector< double> data( 10);
std::generate(data.begin(), data.end(), std::rand); 

distributed::bitonic_sort( world, data.begin(), data.end());

std::stringstream ss;
ss << "Rank: " << world.rank() << " ";
for( auto& i : data){
ss << i << " ";
}
ss << std::endl;

return 1;
}

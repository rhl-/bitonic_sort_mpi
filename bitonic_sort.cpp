#define DEBUG_BITONIC
#include <boost/mpi.hpp>
//#include <diy/serialization.hpp>
#include "bitonic_sort.hpp"
#include <vector>
#include <random>
#include <sstream>
#include <algorithm>

typedef std::vector< int> Vector;
namespace mpi = boost::mpi;

template< typename Communicator, typename Data>
void gather_data( Communicator& world, Data& data, Data& gathered_data){
std::vector< Data > orig_data;
std::vector< Data > ordered_orig_data;
data.push_back( world.rank());
mpi::gather( world, data, orig_data, 0);
data.pop_back();
ordered_orig_data.resize( orig_data.size());
if( world.rank() == 0){
for( auto& p : orig_data){
    int rank = p.back();
    p.pop_back();
    ordered_orig_data[ rank] = std::move( orig_data[ rank]);
}
for( auto& i : ordered_orig_data){ 
	gathered_data.insert( gathered_data.end(), i.begin(), i.end()); 
}
}
}

template< typename Communicator, typename Data>
void generate_correct_answer( Communicator& world, 
			      Data& data,
			      Data& correct_answer){
gather_data( world, data, correct_answer);
std::sort( correct_answer.begin(), correct_answer.end());
}

template< typename Communicator>
void sort_problem(Communicator& world, std::size_t n){
std::random_device rd;
std::mt19937 gen(rd());
std::uniform_int_distribution<> dis(1, .5e9);
std::vector< int> data( n); 
for( auto& i : data){ i = dis(gen); }
distributed::bitonic_sort( world, data);
}


int main( int argc, char** argv){
mpi::environment environment( argc, argv);
mpi::communicator world;
mpi::timer t;

for(auto i = 4; i < 9; ++i){
	t.restart();
	sort_problem( world, std::pow(10, i));
	double time = t.elapsed();
	double max_time = 0;
	mpi::reduce( world, time, max_time, mpi::maximum< double>(), 0);
	if( world.rank() ==0){
		std::cout << world.size() << " " << std::pow(10,i) << " " << max_time << std::endl << std::flush;
	}
}

//std::vector< int> computed_answer;
//gather_data( world, data, computed_answer);
//if(world.rank() == 0){
//std::cout << std::endl;
//if(computed_answer == correct_answer){ std::cout << "ANSWER IS RIGHT!" << std::endl;}
//else{ 
//std::cout << "ANSWER IS WRONG!" << std::endl << std::endl;
//for( auto & i : computed_answer){ std::cout << i << " "; }
//std::cout << std::endl;
//for( auto & i : correct_answer){ std::cout << i << " "; }
//std::cout << std::endl;
//
//}
//} 
return 0;
}

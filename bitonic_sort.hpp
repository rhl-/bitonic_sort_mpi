#ifndef BITONIC_SORT_HPP
#define BITONIC_SORT_HPP

#include <algorithm>
#include <iterator>
#include <cmath>
#include <cassert>
#include <vector>


namespace distributed{

template< typename Communicator>
void print_partners( Communicator& world, std::size_t partner){
  for( auto r = 0; r < world.size(); ++r){
	if( r == world.rank()){
		std::cout << world.rank() << "-" << partner << " |  " << std::flush;
	}
	world.barrier();
   }
   world.barrier();
   if( world.rank() == 0){ std::cout << std::endl << std::flush; }
   world.barrier();
}
template< typename Communicator, typename Iterator>
void print_list( Communicator & world, Iterator begin, Iterator end){
for( auto r = 0; r < world.size(); ++r){
	if( r == world.rank()){
	for( auto i = begin; i != end; ++i){ std::cout << *i << " " << std::flush;}
	if( r != world.size()-1){ std::cout << "\b|" << std::flush; }
	}
	world.barrier();
}
if( world.rank() == 0){ std::cout << std::endl << std::flush; }
world.barrier();
}



template< typename Communicator, typename Iterator, typename Less>
void compare_low(Communicator& world, int partner, 
		 Iterator begin, Iterator end, const Less& less) {
}

template< typename Communicator, typename Iterator, typename Less>
void compare_high(Communicator& world, int partner, 
		 Iterator begin, Iterator end, const Less& less) {
}

template< typename Communicator, typename Iterator, 
	  typename Less=std::less< typename std::iterator_traits< Iterator>::value_type>  >
void bitonic_sort(Communicator& world, Iterator begin, Iterator end, Less less=Less()){

std::sort(begin, end, less);

for (auto i = 2; i <= world.size(); i*=2) {
    int ith_bit = (world.rank() & i);
    int stages = (int)log2(i);
    for (auto stage = 0, j = (1 << (stages-1)); stage < stages; ++stage, j >>=1){
    	int partner = world.rank()^j; //partner is process differing in bit j
        #ifdef DEBUG_BITONIC
	if( world.rank() == 0){ std::cout << "i = " << i << std::endl << std::flush; }
	if( world.rank() == 0){ std::cout << "j = " << j << std::endl << std::flush; }
	world.barrier();
	print_partners( world, partner);
	print_list( world, begin, end);
        #endif
			
	if( ith_bit == 0){
	  //Increasing
 	  if(world.rank() < partner){ 
             compare_low(world, partner, begin, end, less);
	  }else{
	     compare_high(world, partner, begin, end, less);
	  }
	}else{
	  //Decreasing 
	  if(world.rank() > partner){ 
             compare_low(world, partner, begin, end, less); 
	  }else{
	     compare_high(world, partner, begin, end, less);
	  }
	}
        #ifdef DEBUG_BITONIC
	print_list( world, begin, end);
	if( world.rank() == 0){ 
		std::cout << std::endl 
		<< "-----------------------------------------------------------"  
		<< std::endl << std::flush; }
	world.barrier();
        #endif
 
    }
}

} //end namespace distributed


}  

#endif //BITONIC_SORT_HPP

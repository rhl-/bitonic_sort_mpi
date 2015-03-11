/*
* Copyright 2015 Ryan H. Lewis
* This code is released under the BSD see LICENSE
*/
#ifndef BITONIC_SORT_HPP
#define BITONIC_SORT_HPP

#include <algorithm>
#include <iterator>
#include <cmath>
#include <cassert>
#include <vector>


namespace distributed{

namespace detail {
constexpr bool UP =1;
constexpr bool DOWN=0;
bool is_power_two( std::size_t x){ return !(x & x - 1) && x ; }

std::size_t power_two_below( std::size_t x){ 
	std::size_t r=2; 
	while( r < x){ r*=2; } return r/2;  
}

std::size_t power_two_above( std::size_t x){
	return 2*power_two_below( x); 
}


} //end namespace detail

namespace debug{
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
}

template< typename Communicator, 
	  typename Vector,
	  typename Vector1>
void exchange_data( Communicator& world, std::size_t partner, 
		    Vector& send_buffer, Vector1& receive_buffer){
   if( partner >= world.size()){
	std::cout << partner << " " << world.size() << std::endl; 
   }
   assert( partner < world.size());
   //Boost::mpi does not yet implement sendrecv
   boost::mpi::request requests[ 2];
   requests[ 0] = world.isend( partner, 0, send_buffer);
   requests[ 1] = world.irecv( partner, 0, receive_buffer); 
   boost::mpi::wait_all( requests, requests+2);
}
/*
* TODO: We may save a factor of 2 when exchanging larger sets by having one processor
* send its data to the other, having that one merge, then the other processor returning
* the remaining elements. Then we only move |A|+|B| at most instead of sending |A|+|B|
* always, and assuming a \alpha+\beta{n} model, we may potentially hide the latency term \alpha
*/
template< bool direction,
	  typename Communicator, 
	  typename Vector, 
	  typename Vector1, 
	  typename Less>
void exchange_and_merge( Communicator& world, 
			 std::size_t partner,
			 Vector& our_data, 
			 Vector1& their_data, 
			 Less& less){ 
  typedef typename Vector::value_type T;
  exchange_data( world, partner, our_data, their_data); 
  std::vector< T> full_list( our_data.size() + their_data.size());
  T max_min = std::max( our_data.front(), their_data.front());
  T min_max = std::min( our_data.back(), their_data.back());
  std::merge( our_data.begin(), our_data.end(), 
	      their_data.begin(), their_data.end(), 
	      full_list.begin(), less);
  our_data.clear();
  /* When processors do not have the same amount of data on a node the correctness
   * of the overall sort depends critically on the exchange-merge.
   * Suppose A and B are the sets in question, where A is held by low processor and B by high processor
   * Define L_{A,B} = {x | x < max( min(A), min(B))}
   * and U_{A,B} = {x | x > min(max(A),max(B)}
   * We need that L goes to processor holding A and U goes to processor holding B
   * Notice that |L| may be larger than |A| and similarly for |U|. 
   */
  //This branch is removed by the compiler
  if( direction == detail::DOWN){
     int j = 0;
     auto i = full_list.begin();
     while (((*i < max_min) || (j < full_list.size()/2)) && (*i <= min_max)){ 
       our_data.push_back( *i);
       ++i; ++j;
     }
  }else{
     int j = full_list.size()-1; 
     auto i = full_list.rbegin();
     while( (*i >= max_min) && ((j >= full_list.size()/2) || (*i > min_max))){ 
	our_data.push_back( *i);
	++i; --j;
     }
     std::reverse( our_data.begin(), our_data.end());
  }
  their_data.clear();
} 

template< typename Communicator, typename Range, typename Less> 
void bitonic_sort_binary(Communicator& world, Range& our_data, Less& less){
  typedef typename Range::value_type T;
  std::vector< T> their_data;
  for (std::size_t i = 2; i <= world.size(); i*=2){
      bool ith_bit_unset = ((world.rank() & i) == 0);
      bool ith_bit_set = !ith_bit_unset;
      std::size_t l = log2( i);
      for (std::size_t j = (1 << (l-1)); j > 0; j >>= 1) {
       std::size_t partner = world.rank() ^ j;
       bool jth_bit_unset = (world.rank() < partner);
       bool jth_bit_set = !jth_bit_unset;

       //if( world.rank() == 0){ std::cout << "l = " << l <<std::endl; }
       //debug::print_partners( world, partner);
       //debug::print_list( world, our_data.begin(), our_data.end());
	//either we are to the left of our neighbor and ith_bit_unset
	//or we are to the right of our neighbor and the ith_bit_set	
       if ( (jth_bit_unset && ith_bit_unset) || (ith_bit_set && jth_bit_set)){ 
          exchange_and_merge< detail::DOWN>( world, partner, 
             	         		     our_data, their_data, 
					     less);
       }else{
         exchange_and_merge< detail::UP>( world, partner, 
					  our_data, their_data, 
					  less);
      }
      // debug::print_list( world, our_data.begin(), our_data.end());
      // if( world.rank() == 0){ std::cout << "----------------------------------"
      //  				 << std::endl << std::flush; }
      // world.barrier();
    } //end decreasing 
  } //end outer loop
}

template <typename Communicator, typename Vector, typename Less>
void bitonic_merge_increasing( Communicator& world, 
			       Vector& our_data, 
			       Less& less){
  typedef typename Vector::value_type T;
  std::size_t  num_left = detail::power_two_below( world.size());
  std::size_t  num_right = world.size() - num_left;
  std::size_t rank = world.rank(); 
  std::vector< T> their_data;
  // 1, Do merge between the k right procs and the highest k left procs.
  if ( (rank < num_left) && ( rank >= (num_left - num_right) ) ){
    std::size_t partner = rank + num_right;
    assert( partner < world.size());
    exchange_and_merge< detail::DOWN>( world, partner, our_data, their_data, less);
  } else if (rank >= num_left) {
    std::size_t partner = rank - num_right;
    assert( partner < world.size());
    exchange_and_merge< detail::UP>( world, partner, our_data, their_data, less);
  }
}

template< typename Communicator, typename Range, typename Less> 
void bitonic_sort(Communicator& world, Range& range, Less& less){
assert( range.size() > 0);
auto begin = std::begin( range);
auto end = std::end( range);
std::sort( begin, end, less);
if( detail::is_power_two( world.size())){ 
	bitonic_sort_binary( world, range, less);
	return;
}

std::size_t previous_power_two = detail::power_two_below( world.size());
bool in_power_of_two = (world.rank() < previous_power_two);


//we set the processors rank in the new communicator
//so that the first power of two processors keep there ranks
//the remaining ones reindex so that the new proc 0 
//is the one who previously had rank world.size()-1
std::size_t key = world.rank();
if( !in_power_of_two){ key = world.size()-world.rank()-1; }

auto new_world_ = world.split( in_power_of_two, key);
//------------------------------------
//Code to see that the key mapping works
//-------------------------------------
//for( auto i = 0; i < world.size(); ++i){
//if( i == world.rank()){
//std::cout << world.rank() << " ---->> " << key << " ---->> " << new_world_.rank() << std::endl;
//}
//world.barrier();
//}
//if( world.rank() == 0){ std::cout << "------------" << std::endl; }
//world.barrier();
 
if( in_power_of_two){ //ranks 0...2^k --> 0 ...2^k
 bitonic_sort_binary( new_world_, range, less);
}else{  //flip the ranks of these processes
	//2^k+1 ... p --> 2^k .... 0
 bitonic_sort( new_world_, range, less);
 //this way all of this data is sorted in the reverse order!
}

//now we perform a bitonic merge
bitonic_merge_increasing( world, range, less);
//split the processes again into the same groups, 
//this time do not reorder ranks.
auto new_world = world.split( in_power_of_two, world.rank());
if( in_power_of_two){
 bitonic_sort_binary( new_world, range, less);
}else{
 bitonic_sort( new_world, range, less);
}

}

template< typename Communicator, typename Range>
void bitonic_sort(Communicator& world, Range& range){
  typedef typename Range::value_type T;
  std::less< T> less;
  bitonic_sort( world, range, less); 
} 

} //end namespace distributed

#endif //BITONIC_SORT_HPP

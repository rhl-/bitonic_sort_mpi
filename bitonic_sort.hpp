#ifndef BITONIC_SORT_HPP
#define BITONIC_SORT_HPP

#include <algorithm>
#include <iterator>
#include <math.h>
#include <vector>


namespace distributed{

template< typename Communicator, typename Iterator, typename Less>
void compare_low(Communicator& world, int neighbor, 
		 Iterator begin, Iterator end, Less& less) {

    typedef typename Iterator::value_type T; 
    typedef std::vector< T> Buffer;

    std::size_t length = std::distance( begin, end);
    // Exchange with a neighbor whose processor number differs only at the jth bit.
 
    /* Sends the biggest of the list and receive the smallest of the list */
    // Before sending the entire array to partner just send our maximum value,
    //and get there minimum value..
     T min, max = *(begin+length-1);
    //send our greatest element to our neighbor in compare_high()
    auto send_max_request = world.isend( neighbor, 0, max);
    //Wait until we have received min value..
    auto min_receive_request = world.irecv( neighbor, 0, min);

    min_receive_request.wait();
    //Only send to partner numbers which are >= there min value
    auto not_less_than_begin = std::lower_bound( begin, end, min, less);

    //Prepare and send the send buffer     
    Buffer merge_send_buffer( not_less_than_begin, end);
    std::size_t size_to_overwrite = merge_send_buffer.size();
    auto send_merge_request = world.isend( neighbor, 1, merge_send_buffer);
 
    //Prepare receive buffer and wait for it to appear
    Buffer merge_receive_buffer;
    world.recv( neighbor, 1, merge_receive_buffer);
 
    //TODO: OPTIMIZATION: Use the original datastructure as the receive buffer..  
    auto b = merge_receive_buffer.begin();  
    std::copy( b, b+size_to_overwrite, not_less_than_begin);

    //Do not move on until we are done. 
    send_max_request.wait();  
    send_merge_request.wait();
}
///////////////////////////////////////////////////
// Compare High
///////////////////////////////////////////////////
template< typename Communicator, typename Iterator, typename Less>
void compare_high(Communicator& world, int neighbor, 
		 Iterator begin, Iterator end, Less& less) {

    typedef typename Iterator::value_type T; 
    typedef std::vector< T> Buffer;

    std::size_t length = std::distance( begin, end);
    // Exchange with a neighbor whose processor number differs only at the jth bit.
 
    // Before sending the entire array to partner just send our minimum value,
    //and get there maximum value..
     T min = *begin;
     T max;
    //receive maximum element from neighbor in compare_loew()
    auto receive_max_request = world.irecv( neighbor, 0, max);

    //send our greatest element to our neighbor in compare_low()
    auto send_min_request = world.isend( neighbor, 0, min);

    //Wait until we have received min value..
    receive_max_request.wait();

    //Only consider values which are >= there min value
    auto not_less_than_max_begin = std::lower_bound( begin, end, max, less);
    //Compute the size of the potential overlap 
    std::size_t 
     my_overlap_size = std::distance( begin, not_less_than_max_begin);

    //Wait to receive data from partner
    Buffer receive_merge_buffer; 
    world.recv( neighbor, 1, receive_merge_buffer);

    //Once it is received calculate the size
    std::size_t there_overlap_size = receive_merge_buffer.size();

    //Merge there data with our data
    Buffer merged_results( there_overlap_size+my_overlap_size);
    auto new_data_end = std::merge( begin, not_less_than_max_begin, 
			    receive_merge_buffer.begin(),
	 		    receive_merge_buffer.end(), merged_results.begin()); 
    auto new_data_begin =  new_data_end-my_overlap_size;
    //overwrite the original data in tail with the new data 
    std::copy(  new_data_begin, new_data_end, begin); 

    //[begin,end) now contains sorted data..
    //the receive buffer now becomes the send buffer
    receive_merge_buffer.erase( new_data_begin, receive_merge_buffer.end()); 

    auto send_merge_request = world.isend( neighbor, 1, receive_merge_buffer);
 
    //Do not move on until we are done. 
    send_min_request.wait();  
    send_merge_request.wait();
}



template< typename Communicator, typename Iterator, 
	  typename Less=std::less< typename std::iterator_traits< Iterator>::value_type>  >
void bitonic_sort(Communicator& world, Iterator begin, Iterator end, Less less=Less()){

std::sort(begin, end, less);

int dimensions = (int)log2(world.size());

for (auto i = 0; i < dimensions; ++i) {
    bool window_id_parity = ((world.rank() >> (i+1))%2);

    for (auto j = i; j >= 0; --j) {
	bool jth_bit_set = ((world.rank() >> j) % 2);
    	int neighbor = world.rank()^(1 << j);
        // OR (window_id is odd AND jth bit of process is 1)
        if ((window_id_parity == 0 && jth_bit_set == 0)
	 || (window_id_parity != 0 && jth_bit_set != 0)){
            compare_low(world, neighbor, begin, end, less);
        } else {
            compare_high(world, neighbor, begin, end, less);
        }
    }
}

} //end namespace distributed


}  

#endif //BITONIC_SORT_HPP

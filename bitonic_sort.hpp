#ifndef BITONIC_SORT_HPP
#define BITONIC_SORT_HPP

#include <algorithm>
#include <iterator>
#include <cmath>
#include <cassert>
#include <vector>


namespace distributed{

template< typename Communicator, typename Iterator, typename Less>
void compare_low(Communicator& world, int neighbor, 
		 Iterator begin, Iterator end, const Less& less) {

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
    auto min_receive_request = world.irecv( neighbor, 0, min);
    //Prepare receive buffer 
    Buffer merge_receive_buffer;
    //Post our receive very very early
    auto sorted_tail_data_request = world.irecv( neighbor, 1, merge_receive_buffer);
    //Wait until we have received min value..
    min_receive_request.wait();
    //Only send to partner numbers which are >= there min value
    auto not_less_than_begin = std::lower_bound( begin, end, min, less);

    //Prepare and send the send buffer
    Buffer merge_send_buffer( not_less_than_begin, end);
    std::size_t size_to_overwrite = merge_send_buffer.size();
    auto send_merge_request = world.isend( neighbor, 1, merge_send_buffer);
    //Now wait for receive buffer to appear and send buffer to send
    std::vector< boost::mpi::request> requests; 
    requests.push_back( sorted_tail_data_request);
    requests.push_back(send_merge_request);
    boost::mpi::wait_all( requests.begin(), requests.end());

    //We don't want to overwrite the send buffer until the message was sent
    //TODO: OPTIMIZATION: Somehow use the original datastructure as the receive buffer..  
    auto b = merge_receive_buffer.begin();  
    std::copy( b, b+size_to_overwrite, not_less_than_begin);

    //Do not move on until we are done. 
    send_max_request.wait();  
    std::sort(begin, end, less);
}
///////////////////////////////////////////////////
// Compare High
///////////////////////////////////////////////////
template< typename Communicator, typename Iterator, typename Less>
void compare_high(Communicator& world, int neighbor, 
		 Iterator begin, Iterator end, const Less& less) {

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
    //Create merge receive buffer (used below)
    Buffer receive_merge_buffer;  
    //Post receive for buffer extremely early..
    auto receive_merge_request = world.irecv( neighbor, 1, receive_merge_buffer);

    //Wait until we have received max value..
    receive_max_request.wait();
 
    //Only consider values which are <= there max value
    auto greater_than_max_begin = std::upper_bound( begin, end, max, less);

    //Compute the size of the potential overlap 
    std::size_t 
     my_overlap_size = std::distance( begin, greater_than_max_begin);
    //Wait to receive data from partner
    receive_merge_request.wait();
 
    //Once it is received calculate the size
    std::size_t there_overlap_size = receive_merge_buffer.size();

    //Merge there data with our data
    Buffer merged_results( there_overlap_size+my_overlap_size);
    auto new_data_end = std::merge( begin, greater_than_max_begin, 
			    receive_merge_buffer.begin(),
	 		    receive_merge_buffer.end(), merged_results.begin()); 
    auto new_data_begin =  new_data_end-my_overlap_size;
    bool correct_sizes = (std::distance( new_data_begin, new_data_end) == my_overlap_size);
    //overwrite the original data in tail with the new data 
    std::copy(  new_data_begin, new_data_end, begin); 
 
    //[begin,end) should now contains sorted data..

    //we erase the redundant data from the merge buffer, the remaining data we
    //may return to the other process.. 
    merged_results.erase(new_data_begin, new_data_end); 
    receive_merge_buffer.swap( merged_results);

    auto send_merge_request = world.isend( neighbor, 1, receive_merge_buffer);
    
    boost::mpi::request reqs[2];
    reqs[ 0] = send_merge_request;
    reqs[ 1] = send_min_request;
 
    boost::mpi::wait_all( reqs, reqs+1);
    std::sort(begin, end, less);
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

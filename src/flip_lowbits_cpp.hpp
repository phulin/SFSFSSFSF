#ifndef LIB_AVCODEC
extern "C" {
	//libavcodec
	#include <avcodec.h>
	#include <avformat.h>
}
#endif // LIB_AVCODEC

#ifndef STL
//STL
#include <vector>
#include <iterator>
#include <algorithm>
#include <string>
#endif //STL

#ifndef BOOST
//boost
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#endif //BOOST

using namespace std;
using namespace boost::lambda;

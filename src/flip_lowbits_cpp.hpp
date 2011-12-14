#ifndef LIB_AVCODEC
extern "C" {
	//libavcodec
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}
#endif // LIB_AVCODEC

#ifndef STL
//STL
#include <vector>
#include <iterator>
#include <algorithm>
#include <string>
#include <iostream>
#include <fstream>
#include <ios>
#endif //STL

#ifndef BOOST
//boost
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#endif //BOOST

using namespace std;
using namespace boost::lambda;

#include "flip_lowbits_cpp.hpp"
#include <iostream>

class AVInterface{
	vector<string> filenames;
	
	public:
		AVInterface(){
			av_register_all();
		}
		
		~AVInterface(){
			//close file handlers
			//for_each(filenames.begin(), filenames.end(), dosomething(_1));
		}
		
	private:
		
};


int main(int argc, char *argv[]){
	vector<string> filenames;
	
	if (argc>1){
		for(;argc>1;argc--){
			filenames.insert(filenames.begin(), string(argv[argc-1]));
		}
	}
	else{
		cout<<"Hello There, I need some filenames to start working"<<endl;
		//TODO: Usage Info Printout
		return 0;
	}

	//TODO: Remove me
	cout<<"Filenames given:"<<endl;
	for_each(filenames.begin(), filenames.end(), cout << _1 << "\n");

	//unsure if needed
	avcodec_init();
	
	av_register_all();
	
}

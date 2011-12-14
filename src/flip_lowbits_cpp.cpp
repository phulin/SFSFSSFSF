#include "flip_lowbits_cpp.hpp"
#include <iostream>
#include <map>
#include <sstream>
#include <istream>
#include <ostream>
//#include <stringstream>



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

	//serialize vector<string>. Can be extended for other types
	// ofstream out("file.dat");
	

	// out<<"Filenames given:"<<endl<<flush;
	// copy(filenames.begin(), filenames.end(), ostream_iterator<string>(out, "\n"));
	// out<<flush;
	// out.close();


	// //unserialize into vector of strings
	// string line;
	// vector<string> foo;

	// ifstream in("file.dat");

	// while(!in.eof()){
	// 	getline(in, line);
	// 	foo.insert(foo.end(), line);
	// }

	// //test
	// copy(foo.begin(), foo.end(), ostream_iterator<string>(cout, "\n"));


	// //unsure if needed
	// avcodec_init();
	
	// av_register_all();

	//init for serialization
	map<string,string> key_rpath_map;
	key_rpath_map["hello"] = "we b/ww abw//w ba/";
	key_rpath_map["goodbye"] = "foodler/";


	//serialization
	string buf, tmp1, tmp2;
	
	int i, key_rpath_map_size;
	stringstream SuperBlock (stringstream::in | stringstream::out);
	


 
	SuperBlock << key_rpath_map.size() <<endl;


	for(std::map<string, string>::iterator itr = key_rpath_map.begin(), itr_end = key_rpath_map.end(); itr != itr_end; ++itr) {
		tmp1 = itr->first;
		tmp2 = itr->second;
		SuperBlock<<tmp1<<endl<<tmp2<<endl;
 }

	while(SuperBlock.good()){
		getline(SuperBlock, tmp1);
		SuperBlock>>ws;
		buf+=tmp1+"\n";
	}
	printf("%s",buf.c_str());
	

	//test deserialization
	map<string,string> key_rpath_map2;

	stringstream SuperBlock2 (stringstream::in | stringstream::out);
	SuperBlock2<<buf.c_str();
	i = 0;
	key_rpath_map_size = 0;
	tmp1 = "";
	tmp2 = "";

	SuperBlock2 >> key_rpath_map_size;

	cout<<endl;
	for (i = 0; i < key_rpath_map_size; i++){
		SuperBlock2>>ws;
		getline(SuperBlock2, tmp1);
		getline(SuperBlock2, tmp2);
		key_rpath_map2[tmp1] = tmp2;
	}

	cout << endl << endl;


	string foo = key_rpath_map2["goodbye"];
	//	cout<<foo<<endl;
	
	for(std::map<string, string>::iterator itr = key_rpath_map2.begin(), itr_end = key_rpath_map2.end(); itr != itr_end; ++itr) {
		cout<<itr->first<<"|"<<endl;
		cout<<itr->second<<"||"<<endl;
		
	}

	return 0;
}

#include "curl_easy.h"
#include <iostream>
#include <fstream>
#include <string>

void process_url(std::string url)
{
	std::ofstream ofile;
	ofile.open("out.txt");
	
	curl_writer writer (ofile);	// create writer
	curl::curl_easy easy (writer);	// pass to easy constructor
	
	easy.add(curl_pair<CURLoption, std::string>(CURLOPT_URL, url));
	easy.add(curl_pair<CURLoption, long>(CURLOPT_FOLLOWLOCATION, 1L)); 
	
	try
	{
		easy.perform();
	}
	catch (curl_easy_exception error)
	{
		vector<pair<std::string, std::string>> errors = error.what();	
	}
	ofile.close();
}
int main (int argc, char* argv[])
{
	process_url(argv[1]);
	return 0;
}

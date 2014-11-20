/*
	Compile with: g++ crawl.cpp -o crawl -lcurl -std=c++11

*/
#include <stdio.h>
#include <string>
#include <curl/curl.h>

#define CURL_DEPTH 10
int depth = 0;

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	int written = fwrite(ptr, size, nmemb, (FILE *)stream);
	return written;	
}

int mCurl (char* url)
{
	CURL *curl_handle;
	CURLcode res;
	std::string headerFilename = std::to_string(depth) + "_" + std::string(url) + "_head.out";
	std::string bodyFilename = std::to_string(depth) + "_" + std::string(url) + "_body.out";
	FILE *headerFile;
	FILE *bodyFile;

	curl_global_init(CURL_GLOBAL_ALL);
	curl_handle = curl_easy_init();
	if(curl_handle)
	{

		/////////////////////////////////////////////
		//	SET UP CURL
		fprintf(stdout, "url: %s\n", url);
		curl_easy_setopt(curl_handle, CURLOPT_URL, url);
		//curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);	
		curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_data);


		/////////////////////////////////////////////
		//	SET UP FILES
		headerFile = fopen(headerFilename.c_str(), "wb");
		if (headerFile == NULL)
		{
			curl_easy_cleanup(curl_handle);
			return -1;
		}
		bodyFile = fopen(bodyFilename.c_str(), "wb");
		if (bodyFile == NULL)
		{
			curl_easy_cleanup(curl_handle);
			return -1;
		}
		curl_easy_setopt(curl_handle, CURLOPT_HEADERDATA, headerFile);
		curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, bodyFile);


 		/////////////////////////////////////////////
		//	PERFORM THE CURL
		res = curl_easy_perform(curl_handle);


		
		if (res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}
		curl_easy_cleanup(curl_handle);
	}	
	return 0;

}
int main (int argc, char* argv[])
{
	mCurl(argv[1]);	
}

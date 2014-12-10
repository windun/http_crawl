#include <cstring>
#include <iostream>	
#include <curl/curl.h>	
#include <stdlib.h>	// realloc, malloc

struct MemoryStruct {
  char *memory;
  size_t size;
};

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory! */ 
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

std::string POST (std::string post_msg, std::string url)
{
	CURL *curl_handle;
	CURLcode res;
	struct curl_slist *headers = NULL;
 
	struct MemoryStruct chunk;

	chunk.memory = (char *)malloc(1);  /* will be grown as needed by the realloc above */ 
	chunk.size = 0;    /* no data at this point */ 

	headers = curl_slist_append(headers, "Accept: application/json; charset=UTF-8");
	headers = curl_slist_append(headers, "Content-Type: application/json");

	curl_handle = curl_easy_init();
	if(curl_handle) {
		curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());			// url set
		curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, post_msg.c_str());		// send POST message
		//curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);	// write response to memory
		//curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);		// write response memory is &chunk
		
		res = curl_easy_perform(curl_handle);
		if(res != CURLE_OK)
		{
			std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
		}
		else
		{
			std::cout << "Received " << chunk.size << " bytes.\n";
		}
		curl_easy_cleanup(curl_handle);	
	}
	if(chunk.memory) free(chunk.memory);
	curl_global_cleanup();
	return "";
}

int main(void)
{
	POST ("http://localhost:7474/db/data/transaction/commit", "google.com");
}
